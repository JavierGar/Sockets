/***************************************************************************
 *            fwClient.h
 *
 *  Copyright  2016  mc
 *  <mcarmen@<host>>
 ****************************************************************************/
#include "fwClient.h"

/**
 * Function that sets the field addr->sin_addr.s_addr from a host name 
 * address.
 * @param addr struct where to set the address.
 * @param host the host name to be converted
 * @return -1 if there has been a problem during the conversion process.
 */
int setaddrbyname(struct sockaddr_in *addr, char *host)
{
  struct addrinfo hints, *res;
	int status;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM; 
 
  if ((status = getaddrinfo(host, NULL, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    return -1;
  }
  
  addr->sin_addr.s_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
  
  freeaddrinfo(res);
    
  return 0;  
}


/**
 * Returns the port specified as an application parameter or the default port
 * if no port has been specified.
 * @param argc the number of the application arguments.
 * @param an array with all the application arguments.
 * @return  the port number from the command line or the default port if 
 * no port has been specified in the command line. Returns -1 if the application
 * has been called with the wrong parameters.
 */
int getPort(int argc, char* argv[])
{
  int param;
  int port = DEFAULT_PORT;

  optind=1;
  // We process the application execution parameters.
	while((param = getopt(argc, argv, "h:p:")) != -1){
		switch((char) param){
		  case 'h': break;
			case 'p':
			  // We modify the port variable just in case a port is passed as a 
			  // parameter
				port = atoi(optarg);
				break;
			default:
				printf("Parametre %c desconegut\n\n", (char) param);
				port = -1;
		}
	}
	
	return port;
}

/**
 * Returns the host name where the server is running.
 * @param argc the number of the application arguments.
 * @param an array with all the application arguments.
 * @Return Returns the host name where the server is running.<br />
 * Returns null if the application has been called with the wrong parameters.
 */
 char * getHost(int argc, char* argv[]){
  char * hostName = NULL;
  int param;
  
  optind=1;
    // We process the application execution parameters.
	while((param = getopt(argc, argv, "h:p:")) != -1){
		switch((char) param){
			case 'p': break;
			case 'h':
        hostName = (char*) malloc(sizeof(char)*strlen(optarg)+1);
				// Un cop creat l'espai, podem copiar la cadena
				strcpy(hostName, optarg);
				break;
			default:
				printf("Parametre %c desconegut\n\n", (char) param);
				hostName = NULL;
		}
	}
	
	printf("in getHost host: %s\n", hostName); //!!!!!!!!!!!!!!
	return hostName;
 }
 
 
 
/**
 * Shows the menu options. 
 */
void print_menu()
{
		// Mostrem un menu perque l'usuari pugui triar quina opcio fer

		printf("\nAplicació de gestió del firewall\n");
		printf("  0. Hello\n");
		printf("  1. Llistar les regles filtrat\n");
		printf("  2. Afegir una regla de filtrat\n");
		printf("  3. Modificar una regla de filtrat\n");
		printf("  4. Eliminar una regla de filtrat\n");
		printf("  5. Eliminar totes les regles de filtrat.\n");
		printf("  6. Sortir\n\n");
		printf("Escull una opcio: ");
} 


/**
 * Sends a HELLO message and prints the server response.
 * @param sock socket used for the communication.
 */
void process_hello_operation(int sock)
{
	struct hello_rp hello_rp;
	char mensaje [MAX_BUFF_SIZE];
	bzero(mensaje, sizeof(mensaje));
	stshort(MSG_HELLO, mensaje);

	if ((send(sock, mensaje, sizeof(short), 0)) <= -1){
		printf("error en send\n");
		exit(1);
	}
	
	bzero(mensaje, sizeof(mensaje));
	if ((recv(sock, mensaje, sizeof(mensaje), 0)) <= -1){
		printf("error en recv\n");
		exit(1);
	}
	
	hello_rp.opcode = ldshort (mensaje);
	strcpy (hello_rp.msg,(mensaje + 2));
	printf("%d,%s",hello_rp.opcode, hello_rp.msg);
}

void process_list_operation(int sock)
{
  char mensaje[MAX_BUFF_SIZE];
  bzero(mensaje, sizeof(mensaje));
  char* aux; //apuntador que usaremos para movernos por el buffer
  rule elemento_lista;
  int max = 0;
  int i = 1;
  

	stshort (MSG_LIST, mensaje); //guardamos el codigo de operacion y lo enviamos
	if ((send(sock, mensaje, sizeof(short), 0)) <= -1){
		printf("error en send\n");
		exit(1);
	} 

  bzero(mensaje, sizeof(mensaje)); //liberamos el buffer para usarlo para recibir el mensaje
  
    if ((recv(sock, mensaje, sizeof(mensaje), 0)) <= -1){ //recibimos las rules
		printf("error en recv\n");
		exit(1);
    }

	aux = mensaje;
	aux = mensaje+4; //saltamos el espacio de un int

	printf ("-_-_-_-_-Lista de rules-_-_-_-_-\n");

	
	max = ntohs (*((int*) mensaje)); //sacamos el primer int del buffer que sera el numero de rules
	for (i = 0; i<max; i++)
  	{
   		elemento_lista = *((rule*) aux+i); //sacamos la regla de la posicion i y la metemos en un struct rule para tratarla
		printf("%d ",i+1);  //printeamos la posicion en la lista (+1 por que empieza en 0)
		
		if (ntohs(elemento_lista.src_dst_addr) == 0 ) //el usuario a introducido 0 o 1 para determinar src o dst
		{
			printf(" src ");
		}
		else
		{
			printf(" dst ");
		}
		
		printf(" %s/%hu",inet_ntoa(elemento_lista.addr),ntohs(elemento_lista.mask)); //printeamos la direccion y la mascara
		
		if(ntohs(elemento_lista.src_dst_port) == 1 ) //el usuario ha introducido 1 o 2 para sport o dport, y 0 para default, que printeara 2 ceros
		{
			printf(" sport ");
			printf("%hu\n",ntohs(elemento_lista.port));
		}
		else if( ntohs(elemento_lista.src_dst_port) == 2 )
		{
			printf(" dport ");
			printf("%hu\n",ntohs(elemento_lista.port));
		}
		else
		{
			printf(" 0 0\n");
		}

 
	}

	
}

rule introducir_datos(rule new_rule) //funcion para introducir los datos del add o modify que utilizaremos mas tarde
{
	char direccion[MAX_BUFF_SIZE];
    bzero(direccion, sizeof(direccion));
	int i = 1;
	int oct1;
	int oct2;
	int oct3;
	int oct4;
	int puerto;
	
    int tipodir;//determinara si es una IP o una NetID
  
    
	new_rule.src_dst_addr = htons (new_rule.src_dst_addr);
	do
	{	
		i = 1;
		printf("Introduce 0 para una direccion de origen o 1 para una de destino \n");
		scanf("%hu", &new_rule.src_dst_addr);
		if ((new_rule.src_dst_addr != 0) && (new_rule.src_dst_addr != 1))
		{
			printf("Valor no valido \n");
			i=0;
		}

	}while (i!=1);
	new_rule.src_dst_addr = htons (new_rule.src_dst_addr);
	do
	{
		i = 1;
		printf("Introduce 0 para si es una IP o 1 si es una NetID \n");
		scanf("%d", &tipodir);	
		if ((tipodir != 0) && (tipodir != 1))
		{
			printf("Valor no valido \n");
			i=0;
		}
    }while (i!=1);
	do
	{
		i = 1;
		printf("Introduce octeto de la posicion (formato X.-.-.- siendo X un numero del 0 al 255) \n");
		scanf("%d", &oct1);
		if(oct1 < 0 || oct1 > 255)
		{
			printf("Valor no valido \n");
			i = 0;
		}		
		printf("Introduce octeto de la posicion X (formato -.X.-.- siendo X un numero del 0 al 255) \n");
		scanf("%d", &oct2);
		if(oct2 < 0 || oct2 > 255)
		{
			printf("Valor no valido \n");
			i = 0;
		}
		
		printf("Introduce octeto de la posicion (formato -.-.X.- siendo X un numero del 0 al 255) \n");
		scanf("%d", &oct3);
		if(oct3 < 0 || oct3 > 255)
		{
			printf("Valor no valido \n");
			i = 0;
		}
		
		printf("Introduce octeto de la posicion (formato -.-.-.X siendo X un numero del 0 al 255) \n");
		scanf("%d", &oct4);
		if(oct4 < 0 || oct4 > 255)
		{
			printf("Valor no valido \n");
			i = 0;
		}
				
	}while (i!=1);
	
	sprintf (direccion, "%d.%d.%d.%d",oct1,oct2,oct3,oct4);
	printf("la direccion introducida es %s\n", direccion);	
	inet_aton (direccion, &new_rule.addr); 
	
	if (tipodir == 1)
    {
		
		do
		{	
			i=1;
			printf("Introduce el valor de la mascara(entre 0 y 32): \n");
			scanf("%hu", &new_rule.mask);
			if (new_rule.mask > 32 || new_rule.mask < 0)
			{	
				printf("Valor no valido \n");
				i=0;
			}
		}while(i!=1);
		
    }
    else
    {			
    	new_rule.mask = 32;
    }
	new_rule.mask = htons (new_rule.mask);	
	
	do
	{	
		i = 1;
		printf("Introduce 1 para filtrar por puerto origen, 2 para filtrar por puerto destino o 0 para no especificar \n");
	    scanf("%hu", &new_rule.src_dst_port);
		if ((new_rule.src_dst_port != 0) && (new_rule.src_dst_port != 1)&& (new_rule.src_dst_port != 2))
		{
			printf("Valor no valido \n");
			i=0;
		}

	}while (i!=1);
	new_rule.src_dst_port = htons (new_rule.src_dst_port);	
    
	if ((new_rule.src_dst_port != 0))
    {
		do
		{	
				i=1;
				printf("Introduce el puerto(valor entre 0 y 65535) \n");
				scanf("%d", &puerto);
				if (puerto > 65535 || puerto < 0)
				{	
					printf("Valor no valido \n");
					i=0;
				}
		}while(i!=1);
		new_rule.port = (unsigned short) puerto;
		
    }
	else
	{
		new_rule.port = 0;
	}
	new_rule.port = htons (new_rule.port);
	

	
	
	
	
	return new_rule;
}
void process_add_operation(int sock)
{
    char mensaje[MAX_BUFF_SIZE];
    bzero(mensaje, sizeof(mensaje));
	
    	rule new_rule;
	
	new_rule = introducir_datos(new_rule); 
	
	bzero(mensaje, sizeof(mensaje));
	stshort(MSG_ADD, mensaje);
	*((rule *) (mensaje+2)) = new_rule;
	
	if((send (sock, mensaje, sizeof(new_rule)+2, 0)) <= -1)
	{
		printf("Error al enviar la mensaje a añadir \n");
		exit(1);
	}

	process_list_operation(sock);
}
void process_change_operation (int sock)
{	
	char mensaje[MAX_BUFF_SIZE];
	struct mod_rp mod_rp;
	struct con_rp con_rp;
	int confirmacion;
	int posicion;
	rule mod_rule;
	
	process_list_operation(sock);
	printf("Cual quieres cambiar?\n");
	
	bzero(mensaje, sizeof(mensaje));
	stshort(MSG_CHANGE, mensaje);
	
	if(send(sock, mensaje, sizeof(short),0) <= -1)
	{
		printf("Error al enviar el mensaje de modificacion\n");
		exit(1);
	}
	
	do
	{
		bzero(mensaje,sizeof(mensaje));
		if(recv(sock, mensaje, sizeof(mensaje), 0) <= -1)
		{
			printf("Error al recibir\n");
			exit(1);
		}
		
		strcpy (mod_rp.msg,(mensaje));
		printf("%s\n",mod_rp.msg);		
		scanf("%d", &posicion);
		
		if(posicion ==0)
		{
			bzero(mensaje,sizeof(mensaje));
			*((int*) mensaje) = htons (posicion);
			if(send(sock, mensaje, sizeof(posicion), 0) <= -1)
			{
				printf("Error al enviar la posicion");
				exit(1);
			}
			return;
		}
		
		
		bzero(mensaje,sizeof(mensaje));
		*((int*) mensaje) = htons (posicion);
		if(send(sock, mensaje, sizeof(posicion), 0) <= -1)
		{
			printf("Error al enviar la posicion");
			exit(1);
		}
		
		if(recv(sock, mensaje, sizeof(mensaje), 0) <= -1)
		{
			printf("Error al recibir la confirmacion\n");
			exit(1);
		}
		
		confirmacion = ntohs (*((int*)mensaje));
		strcpy (con_rp.msg,(mensaje+ 2));
		printf("%s\n",con_rp.msg);	
		
	}while(confirmacion == 0);
	
		bzero(mensaje,sizeof(mensaje));
		mod_rule = introducir_datos(mod_rule);
		*((rule *) (mensaje)) = mod_rule;
		
		if((send(sock, mensaje, sizeof(rule), 0)<= -1))
		{
			printf("Error al enviar la regla modificada\n");
			exit(1);
		}
	
	
	
}
void process_delete_operation(int sock)
{
	char mensaje[MAX_BUFF_SIZE];
	struct del_rp del_rp;
	struct con_rp con_rp;

	int posicion;
	int confirmacion = 0;
	
	process_list_operation(sock);
	
	bzero(mensaje, sizeof(mensaje));
	stshort(MSG_DELETE,mensaje);

	if((send (sock, mensaje, sizeof(mensaje), 0)) <= -1)
	{
		printf("Error en el send cliente 1\n");
		exit(1);
	}
	
	do
	{
		bzero(mensaje,sizeof(mensaje));
		if(recv(sock, mensaje, sizeof(mensaje), 0) <= -1)
		{
			printf("Error al recibir\n");
			exit(1);
		}
		
		strcpy (del_rp.msg,(mensaje));
		printf("%s\n",del_rp.msg);
		
		scanf("%d", &posicion);
		if(posicion ==0)
		{	
			bzero(mensaje,sizeof(mensaje));
			*((int*) mensaje) = htons (posicion);
			if(send(sock, mensaje, sizeof(posicion), 0) <= -1)
			{
				printf("Error al enviar la posicion");
				exit(1);
			}
			return;
			
		}
		
		
		bzero(mensaje,sizeof(mensaje));
		*((int*) mensaje) = htons (posicion);
		if(send(sock, mensaje, sizeof(posicion), 0) <= -1)
		{
			printf("Error al enviar la posicion");
			exit(1);
		}
		
		if(recv(sock, mensaje, sizeof(mensaje), 0) <= -1)
		{
			printf("Error al recibir la confirmacion\n");
			exit(1);
		}
		
		confirmacion = ntohs (*((int*)mensaje));
		strcpy (con_rp.msg,(mensaje+ 2));
		printf("%s\n",con_rp.msg);	
		
	}while(confirmacion == 0);
	
	
}
/**
 * Closes the socket connected to the server and finishes the program.
 * @param sock socket used for the communication.
 */
void process_exit_operation(int sock)
{
	char mensaje [MAX_BUFF_SIZE];
	bzero(mensaje, sizeof(mensaje));
    stshort (MSG_FINISH, mensaje);

	if ((send (sock, mensaje, sizeof(short), 0 )) <= -1)
	{
		printf ("Error al enviar el mensaje");
		exit(1);
	}
    close(sock);

    exit(1);
}

void process_flush_operation(int sock)
{
	char mensaje[MAX_BUFF_SIZE];
  	bzero(mensaje, sizeof(mensaje));
	
	stshort (MSG_FLUSH, mensaje);

  	if((send (sock, mensaje, sizeof(mensaje), 0)) < 0)
  	{
    	printf("Error al enviar");
  	}

}
	



/** 
 * Function that process the menu option set by the user by calling 
 * the function related to the menu option.
 * @param s The communications socket
 * @param option the menu option specified by the user.
 */
void process_menu_option(int s, int option)
{		
  switch(option){
    // Opció HELLO
    case MENU_OP_HELLO:
    	process_hello_operation(s);
    	break;
    case MENU_OP_LIST_RULES:
        process_list_operation(s);
    	break;
    case MENU_OP_ADD_RULE:
        process_add_operation(s);
    	break;
    case MENU_OP_CHANGE_RULE:
        process_change_operation(s);
    	break;
    case MENU_OP_DEL_RULE:
        process_delete_operation(s);
      	break;
    case MENU_OP_FLUSH:
        process_flush_operation(s);
      	break;
    case MENU_OP_EXIT:
    	process_exit_operation(s);
		break;
    default:
      printf("Invalid menu option\n");
  }
}


int main(int argc, char *argv[]) {
	
	int sockcl;
	unsigned short port;
	char *hostName;
	int menu_option = 0;

	port = getPort(argc, argv);
	hostName = getHost(argc, argv);

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	setaddrbyname(&server, hostName);

	if (hostName == NULL){
		perror("No s'ha especificat el nom del servidor\n\n");
		return -1;
	}

	sockcl = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockcl <= -1) {
		printf("error en creacion de socket()\n");
		exit(1);
	}
	else {  printf("socket creado\n");  }

	if (connect (sockcl, (struct sockaddr*)&server, sizeof(struct sockaddr) ) < 0) {
		printf("connect() error\n");
		exit(1);
	}
	else {  printf("conexion establecida\n");  }

	do {
		print_menu();
		scanf("%d", &menu_option);
		printf("\n\n");
		process_menu_option(sockcl, menu_option);

	} while (menu_option != MENU_OP_EXIT); 

	return 0;
}

