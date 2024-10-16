/****************************************************************************/
/* Plantilla para implementación de funciones del cliente (rcftpclient)     */
/* $Revision$ */
/* Aunque se permite la modificación de cualquier parte del código, se */
/* recomienda modificar solamente este fichero y su fichero de cabeceras asociado. */
/****************************************************************************/

/**************************************************************************/
/* INCLUDES                                                               */
/**************************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "rcftp.h" // Protocolo RCFTP
#include "rcftpclient.h" // Funciones ya implementadas
#include "multialarm.h" // Gestión de timeouts
#include "vemision.h" // Gestión de ventana de emisión
#include "misfunciones.h"

/**************************************************************************/
/* VARIABLES GLOBALES                                                     */
/**************************************************************************/
// elegir 1 o 2 autores y sustituir "Apellidos, Nombre" manteniendo el formato
//char* autores=""; // un solo autor
char* autores="Autor: Moncasi Gos�, Carlota\nAutor: Soriano S�nchez, Paula"; // dos autores

// variable para indicar si mostrar información extra durante la ejecución
// como la mayoría de las funciones necesitaran consultarla, la definimos global
extern char verb;


// variable externa que muestra el número de timeouts vencidos
// Uso: Comparar con otra variable inicializada a 0; si son distintas, tratar un timeout e incrementar en uno la otra variable
extern volatile const int timeouts_vencidos;


/**************************************************************************/
/* Obtiene la estructura de direcciones del servidor */
/**************************************************************************/
	struct addrinfo* obtener_struct_direccion(char *dir_servidor, char *servicio, char f_verbose){
		struct addrinfo hints,     // variable para especificar la solicitud
							*servinfo; // puntero para resp de getaddrinfo()
			struct addrinfo *direccion; // puntero para recorrer la lista de direcciones de servinfo
			int status;      // finalización correcta o no de la llamada getaddrinfo()
			int numdir = 1;  // contador de estructuras de direcciones en la lista de direcciones de servinfo

			// sobreescribimos con ceros la estructura para borrar cualquier dato que pueda malinterpretarse
			memset(&hints, 0, sizeof hints);

			// genera una estructura de dirección con especificaciones de la solicitud
			if (f_verbose)
			{
				printf("1 - Especificando detalles de la estructura de direcciones a solicitar... \n");
				fflush(stdout);
			}

			hints.ai_family = AF_UNSPEC; // opciones: AF_UNSPEC; IPv4: AF_INET; IPv6: AF_INET6; etc.

			if (f_verbose)
			{
				printf("\tFamilia de direcciones/protocolos: ");
				switch (hints.ai_family)
				{
					case AF_UNSPEC: printf("IPv4 e IPv6\n"); break;
					case AF_INET:   printf("IPv4)\n"); break;
					case AF_INET6:  printf("IPv6)\n"); break;
					default:        printf("No IP (%d)\n", hints.ai_family); break;
				}
				fflush(stdout);
			}

			hints.ai_socktype = SOCK_DGRAM; // especificar tipo de socket 

			if (f_verbose)
			{
				printf("\tTipo de comunicación: ");
				switch (hints.ai_socktype)
				{
					case SOCK_STREAM: printf("flujo (TCP)\n"); break;
					case SOCK_DGRAM:  printf("datagrama (UDP)\n"); break;
					default:          printf("no convencional (%d)\n", hints.ai_socktype); break;
				}
				fflush(stdout);
			}

			// flags específicos dependiendo de si queremos la dirección como cliente o como servidor
			if (dir_servidor != NULL)
			{
				// si hemos especificado dir_servidor, es que somos el cliente y vamos a conectarnos con dir_servidor
				if (f_verbose) printf("\tNombre/dirección del equipo: %s\n", dir_servidor); 
			}
			else
			{
				// si no hemos especificado, es que vamos a ser el servidor
				if (f_verbose) printf("\tNombre/dirección: equipo local\n"); 
				hints.ai_flags = AI_PASSIVE; // especificar flag para que la IP se rellene con lo necesario para hacer bind
			}
			if (f_verbose) printf("\tServicio/puerto: %s\n", servicio);

			// llamada a getaddrinfo() para obtener la estructura de direcciones solicitada
			// getaddrinfo() pide memoria dinámica al SO, la rellena con la estructura de direcciones,
			// y escribe en servinfo la dirección donde se encuentra dicha estructura.
			// La memoria *dinámica* reservada por una función NO se libera al salir de ella.
			// Para liberar esta memoria, usar freeaddrinfo()
			if (f_verbose)
			{
				printf("2 - Solicitando la estructura de direcciones con getaddrinfo()... ");
				fflush(stdout);
			}
			status = getaddrinfo(dir_servidor, servicio, &hints, &servinfo);
			if (status != 0)
			{
				fprintf(stderr,"Error en la llamada getaddrinfo(): %s\n", gai_strerror(status));
				exit(1);
			} 
			if (f_verbose) printf("hecho\n");

			// imprime la estructura de direcciones devuelta por getaddrinfo()
			if (f_verbose)
			{
				printf("3 - Analizando estructura de direcciones devuelta... \n");
				direccion = servinfo;
				while (direccion != NULL)
				{   // bucle que recorre la lista de direcciones
					printf("    Dirección %d:\n", numdir);
					printsockaddr((struct sockaddr_storage*) direccion->ai_addr);
					// "avanzamos" a la siguiente estructura de direccion
					direccion = direccion->ai_next;
					numdir++;
				}
			}

			// devuelve la estructura de direcciones devuelta por getaddrinfo()
			return servinfo;
		}
	/**************************************************************************/
	/* Imprime una direccion */
	/**************************************************************************/
	void printsockaddr(struct sockaddr_storage *saddr) {
		struct sockaddr_in  *saddr_ipv4; // puntero a estructura de dirección IPv4
		// el compilador interpretará lo apuntado como estructura de dirección IPv4
		struct sockaddr_in6 *saddr_ipv6; // puntero a estructura de dirección IPv6
		// el compilador interpretará lo apuntado como estructura de dirección IPv6
		void *addr; // puntero a dirección. Como puede ser tipo IPv4 o IPv6 no queremos que el compilador la interprete de alguna forma particular, por eso void
		char ipstr[INET6_ADDRSTRLEN]; // string para la dirección en formato texto
		int port; // para almacenar el número de puerto al analizar estructura devuelta

		if (saddr == NULL)
		{ 
			printf("La dirección está vacía\n");
		}
		else
		{
			printf("\tFamilia de direcciones: ");
			fflush(stdout);
			if (saddr->ss_family == AF_INET6)
			{   //IPv6
				printf("IPv6\n");
				// apuntamos a la estructura con saddr_ipv6 (el cast evita el warning),
				// así podemos acceder al resto de campos a través de este puntero sin más casts
				saddr_ipv6 = (struct sockaddr_in6 *)saddr;
				// apuntamos a donde está realmente la dirección dentro de la estructura
				addr = &(saddr_ipv6->sin6_addr);
				// obtenemos el puerto, pasando del formato de red al formato local
				port = ntohs(saddr_ipv6->sin6_port);
			}
			else if (saddr->ss_family == AF_INET)
			{   //IPv4
				printf("IPv4\n");
				saddr_ipv4 = (struct sockaddr_in *)saddr;
				addr = &(saddr_ipv4->sin_addr);
				port = ntohs(saddr_ipv4->sin_port);
			}
			else
			{
				fprintf(stderr, "familia desconocida\n");
				exit(1);
			}
			// convierte la dirección ip a string 
			inet_ntop(saddr->ss_family, addr, ipstr, sizeof ipstr);
			printf("\tDirección (interpretada según familia): %s\n", ipstr);
			printf("\tPuerto (formato local): %d\n", port);
		}
	}
	/**************************************************************************/
	/* Configura el socket, devuelve el socket y servinfo */
	/**************************************************************************/
	int initsocket(struct addrinfo *servinfo, char f_verbose) {
		int sock;

		printf("\nSe usará ÚNICAMENTE la primera dirección de la estructura\n");

		// crea un extremo de la comunicación y devuelve un descriptor
		if (f_verbose)
		{
			printf("Creando el socket (socket)... ");
			fflush(stdout);
		}
		sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
		if (sock < 0)
		{
			perror("Error en la llamada socket: No se pudo crear el socket");
			/* muestra por pantalla el valor de la cadena suministrada por el programador, dos puntos y un mensaje de error que detalla la causa del error cometido */
			exit(1);
		}
		if (f_verbose) {
			printf("hecho\n");
		}

		if (f_verbose) { 
			printf("Estableciendo la comunicaci�n a trav�s del socket (connect)... "); 
			fflush(stdout); 
		}
		return sock;
	}


	/* Pre: ---
	 * Post : dado un mensaje que hemos recibido, verifica si este
	 * es válido. En el caso de que lo sea devuelve 1. En caso contrario,
	 * devuelve 0.
	*/
	int esMensajeValido(struct rcftp_msg *recvbuffer) { 
		int esperado=1;

		if (recvbuffer->version!=RCFTP_VERSION_1) {//vcomprobamos si la versión incorrecta
			esperado=0;
			fprintf(stderr,"Error: recibido un mensaje con versión incorrecta\n");
		}
		if (issumvalid(recvbuffer,sizeof(*recvbuffer))==0) {//comprobamos si el checksum es incorrecto
			esperado=0;
			fprintf(stderr,"Error: recibido un mensaje con checksum incorrecto\n"); 
		}
		return esperado;
	}

	/* Pre:---
	 * Post: dada una respuesta y un mensaje, verifica si la respuesta "resp" recibida
	 * es la esperada o no, devolviendo 1 si es así, o 0 en caso contrario.
	*/
	int esLaRespEsperada(struct rcftp_msg *resp, struct rcftp_msg *mensaje){
		if (mensaje->flags == F_FIN){
			if (resp->flags == F_FIN){
				return 1;
			}
			else return 0;
		}
		else if (htonl(resp->next) == htonl(mensaje->numseq) + htons(mensaje->len) && (resp->flags != F_BUSY && resp->flags != F_ABORT)) {
			return 1; 
		}
		else return 0;
	}

	/* Pre:---
	 * Post: dada una respuesta y un mensaje, verifica si la respuesta "resp" recibida
	 * es la esperada o no, devolviendo 1 si es así, o 0 en caso contrario.
	*/
	bool esLaRespEsperada3(struct rcftp_msg * res, struct rcftp_msg * men, int tamanyoDoc, bool final){
		/*if (respues->flags == F_FIN){
			return 1;
		}
		if((ntohl(respues->next) > windowstart() && ntohl(respues->next) <= windowend())){
			return 1;
		}
		return 0;*/
		return (res->flags == F_FIN || (ntohl(res->next) > windowstart() && ntohl(res->next) <= windowend()));
	}

	/*
	 * Pre: ---
	 * Post: dado "ult_mensaje" que indica si se ha alcanzado el fin
	 * de fichero, la respuesta "resp", el primer mensaje "primer_msj" y "data", que son los datos que
	 * hemos leído, construye el mensaje "mensaje", también pasado como parámetro rellenando todos y cada uno de sus campos.
	*/
	void construirMensajeRCFTP (struct rcftp_msg *mensaje, struct rcftp_msg *resp, int ult_mensaje, int data, int primer_msj) {
		mensaje->version = 1;
		mensaje->sum = 0;
		if (primer_msj == 1) {
			mensaje->numseq = htonl(0);
		}
		else {
			mensaje->numseq = resp->next;
		}
		mensaje->next = htonl(0);
		mensaje->len = htons(data);
		mensaje->sum = xsum((char*) mensaje, sizeof(*mensaje));
		if (ult_mensaje == 0){
			mensaje->flags = F_NOFLAGS;
		}
	}

	void construirMensaje3 (struct rcftp_msg* mensaje, int totalDatos, int* p) {
		mensaje->version = RCFTP_VERSION_1;
		mensaje->sum = 0;
		mensaje->numseq = htonl(*p);
		mensaje->next = htonl(0);
		mensaje->len = htons(totalDatos);
		mensaje->sum = xsum((char*) mensaje, sizeof(*mensaje));
		/*if (ult_mensaje == 0){
			mensaje->flags = F_NOFLAGS;
		}*/
		*p+=totalDatos;
	}

	/**************************************************************************/
	/*  algoritmo 1 (basico)  */
	/**************************************************************************/
	void alg_basico(int socket, struct addrinfo *servinfo) {

		struct rcftp_msg* mensaje = malloc(sizeof(struct rcftp_msg)); // Mensaje a enviar
		struct rcftp_msg* resp = malloc(sizeof(struct rcftp_msg)); // Respuesta recibida
		struct sockaddr* recibido = NULL; 
		socklen_t long_recibido;
		printf("Comunicación con algoritmo básico\n");

		int sentsize, rcvsize, primer_msj = 1, ult_msj = 0, ult_msj_confirm = 0, datos = readtobuffer((char*)mensaje->buffer, RCFTP_BUFLEN); 

		if(datos <= 0) { //para saber si hemos llegado al final del fichero
			mensaje->flags = F_FIN;
			ult_msj = 1;
		}
		
		construirMensajeRCFTP(mensaje, resp, ult_msj, datos, primer_msj); // construimos el mensaje rellenando sus campos
		primer_msj = 0;
		while (ult_msj_confirm == 0) { 
			sentsize = sendto(socket, (char*) mensaje, sizeof(*mensaje), 0, servinfo->ai_addr, servinfo->ai_addrlen); //el mensaje es enviado
			if (sentsize < 0) {
				perror("Se ha producido un error en la funcion sendto...\n");
				exit(1);
			}

			rcvsize = recvfrom(socket, (char*) resp, sizeof(*resp), 0, recibido, &long_recibido); //el mensaje es recibido
			if (rcvsize < 0) {
				perror("Se ha producido un error en la funcion recvfrom...\n");
				exit(1);
			} 

			if (esMensajeValido(resp) && esLaRespEsperada(resp, mensaje)){ //verificamos  si el mensaje es correcto
				if (ult_msj == 1) { 
					ult_msj_confirm = 1;
					printf("Ultimo mensaje confirm\n");
				}
				else { 
					int datos = readtobuffer((char*)mensaje->buffer, RCFTP_BUFLEN);//leemos del buffer de nuevo y construimos el siguiente mensaje que enviaremos
					if(datos <= 0){
						printf("He terminado\n");
						ult_msj = 1;
						mensaje->flags = F_FIN;
					}
					construirMensajeRCFTP(mensaje, resp, ult_msj, datos, primer_msj);			
					
				}
			}
		}
	}

	/* Pre: ---
	 * Post: dado un socket y un mensaje, hace que sea posible la recepción de la respuesta
	 * del servidor.
	*/
	void recibir(int socket, struct rcftp_msg *mensaje) {
		struct sockaddr* recibido = NULL;
		socklen_t long_recibido;
		int timeouts_procesados = 0;
		int numDatosRecibidos = 0;

		addtimeout();
		int esperar = 1;
		while (esperar) {
			numDatosRecibidos = recvfrom(socket, (char*) mensaje, sizeof(*mensaje), 0, recibido, &long_recibido);
			if (numDatosRecibidos > 0) {
				canceltimeout();
				esperar = 0;
			}
			if (timeouts_procesados != timeouts_vencidos) {
				esperar = 0;
				timeouts_procesados++;
			}
		}
	}

	/**************************************************************************/
	/*  algoritmo 2 (stop & wait)  */
	/**************************************************************************/
	void alg_stopwait(int socket, struct addrinfo *servinfo) {

		struct rcftp_msg* mensaje = malloc(sizeof(struct rcftp_msg)); // Aquí almacenaremos el mensaje a enviar
		struct rcftp_msg* resp = malloc(sizeof(struct rcftp_msg)); // Aquí almacenaremos la resp recibida
		int sockflags; // Estas tres líneas permiten poner el socket en modo NO bloqueante
		sockflags = fcntl(socket, F_GETFL, 0); //Obtiene el valor de los flags
		fcntl(socket, F_SETFL, sockflags | O_NONBLOCK); // Modifica el flag de bloqueo
		signal(SIGALRM, handle_sigalrm); // Para usar los timeouts, tenemos que asociarle a SIGALRM su rutina de captura

		printf("Comunicación con algoritmo stop&wait\n");
		int sntsize, primer_msj = 1, ult_msj = 0, ult_msj_confirm = 0, datos = readtobuffer((char*)mensaje->buffer, RCFTP_BUFLEN); 
		if(datos <= 0) {  //para saber si hemos llegado al final del fichero
			mensaje->flags = F_FIN;
			ult_msj = 1;
		}
		
		construirMensajeRCFTP(mensaje, resp, ult_msj, datos, primer_msj); //construimos el mensaje
		primer_msj = 0;
		while (ult_msj_confirm == 0) {
			sntsize = sendto(socket, (char*) mensaje, sizeof(*mensaje), 0, servinfo->ai_addr, servinfo->ai_addrlen); //el mensaje es enviado
			if (sntsize < 0) {
				perror("Se ha producido un error al enviar\n");
				exit(1);
			}

			recibir(socket, resp);

			if (esMensajeValido(resp) && esLaRespEsperada(resp, mensaje)) { //verificamos  si el mensaje es correcto
				if (ult_msj) { 
					ult_msj_confirm = 1;
				}
				else { //leemos del buffer de nuevo y construimos el siguiente mensaje que enviaremos
					int datos = readtobuffer((char*)mensaje->buffer, RCFTP_BUFLEN);
					if(datos <= 0) {
						ult_msj = 1;
						mensaje->flags = F_FIN;
					}
					
					construirMensajeRCFTP(mensaje, resp, ult_msj, datos, primer_msj);			
					
				}
			}
		}
	
	}

	/**************************************************************************/
	/*  algoritmo 3 (ventana deslizante)  */
	/**************************************************************************/
	void construirMensajeMasViejoDeVentanaEmision (struct rcftp_msg * msj, int longi, bool finEntrada, int windowSize) {

		msj->numseq = htonl (getdatatoresend ((char*) msj->buffer, &longi));
		if (finEntrada && getfreespace() == windowSize){

			msj->flags = F_FIN;
			printf("Se han leído todos los datos de la ventana.\n");
			msj->numseq = htonl (windowstart());
			msj->len = htons (0);

		} else {

			msj->flags = F_NOFLAGS;
			msj->len = htons (longi);

		}
		msj->version = RCFTP_VERSION_1;
		msj->sum = 0;
		msj->next = htonl(0);
		msj->sum = xsum ((char*)msj, sizeof(*msj));

	}


void alg_ventana(int socket, struct addrinfo *servinfo, int windowSize) {

	  printf("Comunicación con algoritmo go-back-n\n");

    signal(SIGALRM, handle_sigalrm);

    int sockflags;
    sockflags = fcntl(socket, F_GETFL, 0); // obtiene el valor de los flags
    fcntl(socket, F_SETFL, sockflags | O_NONBLOCK); // modifica el flag de bloqueo

    // ventana no necesita parametros
    setwindowsize (windowSize);

    struct rcftp_msg * mensaje = malloc(sizeof(struct rcftp_msg)),
                     * respuesta = malloc(sizeof(struct rcftp_msg));
    respuesta -> next = htonl(0);

    bool ultimoMensaje = false, ultimoMensajeConfirmado;

    int bytesLeidos = 1, numDatosRecibidos, timeouts_procesados, numSeq = 0, nMen = 0;

    while (!ultimoMensajeConfirmado) {

        // BLOQUE DE ENVÍO

        if (getfreespace() >= RCFTP_BUFLEN && bytesLeidos > 0) {

            bytesLeidos = readtobuffer ((char*)mensaje->buffer, RCFTP_BUFLEN);

            if (bytesLeidos <= 0) {

                    ultimoMensaje = true;
                    mensaje->flags = F_FIN;
                    printf ("Se ha leído el último mensaje de entrada estándar.\n");

            } else
                    mensaje->flags = F_NOFLAGS;

            construirMensaje3(mensaje, bytesLeidos, &numSeq);

            if (sendto (socket, (char*)mensaje, sizeof(*mensaje), 0, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {

                perror ("Error de envío.\n");
                exit(1);

            }

            addtimeout();

            addsentdatatowindow ((char*)mensaje->buffer, bytesLeidos);

        }

        // BLOQUE DE RECEPCIÓN

        numDatosRecibidos = recvfrom (socket, (char*)respuesta, sizeof(*respuesta), 0, servinfo->ai_addr, &servinfo->ai_addrlen);

        if (numDatosRecibidos > 0) {

            if (esMensajeValido (respuesta) && esLaRespEsperada3 (respuesta, mensaje, numSeq, (bytesLeidos >= 0))) {

                if (respuesta->flags != F_FIN) {

                    canceltimeout();

                    freewindow (htonl (respuesta->next));

                    nMen++;
                    printf ("Envío confirmado: un total hasta ahora de %d confirmaciones y %d bytes enviados.\n", nMen, windowstart());

                } else {

                    ultimoMensajeConfirmado = true;
                    printf ("Se ha confirmado el último envío.\n");

                }

            }

        }

        // BLOQUE DE PROCESADO DE TIMEOUT

        if (timeouts_procesados != timeouts_vencidos) {

            construirMensajeMasViejoDeVentanaEmision(mensaje, RCFTP_BUFLEN, ultimoMensaje, windowSize);

            if (sendto (socket, (char*)mensaje, sizeof(*mensaje), 0, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {

                perror("Error de envío.\n");
                exit(1);

            }

            addtimeout();

            timeouts_procesados++;

        }

    }

}
