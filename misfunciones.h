/****************************************************************************/
/* Plantilla para cabeceras de funciones del cliente (rcftpclient)          */
/* Plantilla $Revision$ */
/* Autor: Carlota Moncasi Gos� (839841), Paula Soriano S�nchez (843710) */
/****************************************************************************/
#include <stdbool.h>

/**
 * Obtiene la estructura de direcciones del servidor
 *
 * @param[in] dir_servidor String con la dirección de destino
 * @param[in] servicio String con el servicio/número de puerto
 * @param[in] f_verbose Flag para imprimir información adicional
 * @return Dirección de estructura con la dirección del servidor
 */
struct addrinfo* obtener_struct_direccion(char *dir_servidor, char *servicio, char f_verbose);

/**
 * Imprime una estructura sockaddr_in o sockaddr_in6 almacenada en sockaddr_storage
 *
 * @param[in] saddr Estructura de dirección
 */
void printsockaddr(struct sockaddr_storage * saddr);

/**
 * Configura el socket
 *
 * @param[in] servinfo Estructura con la dirección del servidor
 * @param[in] f_verbose Flag para imprimir información adicional
 * @return Descriptor del socket creado
 */
int initsocket(struct addrinfo *servinfo, char f_verbose);


/**
 * Algoritmo 1 del cliente
 *
 * @param[in] socket Descriptor del socket
 * @param[in] servinfo Estructura con la dirección del servidor
 */
void alg_basico(int socket, struct addrinfo *servinfo);


/**
 * Algoritmo 2 del cliente
 *
 * @param[in] socket Descriptor del socket
 * @param[in] servinfo Estructura con la dirección del servidor
 */
void alg_stopwait(int socket, struct addrinfo *servinfo);


/**
 * Algoritmo 3 del cliente
 *
 * @param[in] socket Descriptor del socket
 * @param[in] servinfo Estructura con la dirección del servidor
 * @param[in] window Tamaño deseado de la ventana deslizante
 */
void alg_ventana(int socket, struct addrinfo *servinfo, int window);

void construirMensajeRCFTP (struct rcftp_msg *mensaje, struct rcftp_msg *resp, int ult_mensaje, int data, int primer_msj);

void construirMensaje3 (struct rcftp_msg * out, int longitud, int*);

void construirMensajeMasViejoDeVentanaEmision (struct rcftp_msg * out, int longitud, bool finEntrada, int tamanyoVentana);

int esMensajeValido (struct rcftp_msg * in);

int esLaRespEsperada (struct rcftp_msg *resp, struct rcftp_msg *mensaje);

bool esLaRespEsperada3 (struct rcftp_msg * res, struct rcftp_msg * men, int tamanyoDoc, bool final);

