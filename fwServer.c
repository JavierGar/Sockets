/***************************************************************************
 *            fwServer.c
 *
 *  Copyright  2016  mc
 *  <mc@<host>>
 ****************************************************************************/

#include "fwServer.h"	
#include <stdbool.h>

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
	while((param = getopt(argc, argv, "p:")) != -1){
		switch((char) param){
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
 * Function that sends a HELLO_RP to the  client
 * @param sock the communications socket
 */
void process_HELLO_msg(int sock)
{ 
  struct hello_rp hello_rp; //creamos el struct que contendra el op code y el mensaje
  stshort(MSG_HELLO_RP, &hello_rp.opcode); //guardamos un short en el campo opcode
  strcpy(hello_rp.msg, "Hello World"); // guardamos un string con el mensaje en el campo msg
  char mensaje[MAX_BUFF_SIZE]; 
  bzero(mensaje, sizeof(mensaje)); //ponemos todo el buffer con el que enviaremos el mensaje a 0
  *((struct hello_rp*) mensaje) = hello_rp; //metemos los datos del struct dentro del buffer 
  if ((send(sock, mensaje, sizeof(struct hello_rp), 0)) <= -1) //enviamos a traves de sock el buffer mensaje del tamaño struct hello_rp
  {
	  printf("error al enviar hello world\n");
  }
  else
  { 
	  printf("mensaje enviado\n");
  }
}
void process_LIST_msg(int sock, struct FORWARD_chain *chain)
{
   struct fw_rule * actual; //usaremos actual para movernos por la lista
   char* aux; //usaremos aux para movernos por el buffer
   char mensaje[MAX_BUFF_SIZE];
   bzero(mensaje, sizeof(mensaje));
   int i;
   
   actual = chain -> first_rule; //actual es first rule
   aux = mensaje; 
   aux = aux+4; //dejamos un espacio de un int
   
   *((int*) mensaje) = htons (chain->num_rules); //guardamos el numero de rules en el buffer a enviar

   for(i =0; i < chain->num_rules; i++) 
   {
    *(( rule*) aux+i) = actual -> rule; //guardamos la rule actual de la lista en el buffer
      actual = actual->next_rule; //pasamos a la siguiente rule

   }
    if ((send(sock, mensaje, 4+chain -> num_rules * sizeof(rule), 0)) <= -1) //enviamos el buffer con un int del numero de rules y las rules
    {
    printf("error al enviar lista\n");
    exit(1);
    }
}

void process_ADD_msg(int sock, char *buffer, struct FORWARD_chain *chain)
{
	struct fw_rule * new_rule; //se crea el struct para la nueva regla
	struct fw_rule * aux; // se crea el auxiliar que utilizaremos para movernos por las reglas ya existentes
	int last = 0; //int para el bucle que recorre las reglas

	new_rule = malloc(sizeof(struct fw_rule));
	new_rule -> next_rule = NULL;
	new_rule->rule=*((rule *) buffer);
	
	if (chain -> first_rule ==NULL) //si la first_rule esta vacia, la lista esta vacia
	{
		chain -> first_rule = new_rule; //hacemos que la nueva regla sea la primera
		//*((rule *) buffer); //la siguiente regla estará vacia
	}
	else //si no está vacia
	{
		aux = chain -> first_rule; //movemos el aux a la primera regla
		while (!last) //mientras no estemos en la ultima posicion
		{
			if ( aux -> next_rule == NULL) //si la siguiente regla es null, es decir, estamos en la ultima posicion
			{
				aux -> next_rule = new_rule; //la siguiente regla de la actual sera la nueva
				last = 1; //cambiamos el valor de last para salir del bucle
			}
			aux = aux -> next_rule; //el aux pasa a la siguiente regla
		}
	
	}
	chain -> num_rules = chain -> num_rules+1; //el numero de reglas aumenta
}
void process_CHANGE_msg (int sock, struct FORWARD_chain *chain)
{
	char mensaje [MAX_BUFF_SIZE];
	bzero(mensaje, sizeof(mensaje));
	struct fw_rule* aux;
    char* buff;
	bool found = false;
	int i = 1;
	int posicion;
	rule mod_rule;
	int max;

	struct mod_rp mod_rp;
	strcpy(mod_rp.msg, "Inserta la posicion de la entrada a modificar o pulsa 0 para salir\n");
	
	struct con_rp con_rp;
	stshort(1, &con_rp.opcode);
	strcpy(con_rp.msg, "Valor valido\n");	
	
	struct nocon_rp nocon_rp;
	stshort(0, &nocon_rp.opcode);
	strcpy(nocon_rp.msg, "Valor no valido\n");
	
	max = chain->num_rules;
	
	do
	{   
	    bzero(mensaje, sizeof(mensaje));
	    *((struct mod_rp*) mensaje) = mod_rp;
	  
	    if ((send(sock, mensaje, sizeof(struct mod_rp), 0)) <= -1) 
		{
		  printf("error al enviar mensaje de modificacion\n");
	    }
		
		bzero(mensaje, sizeof(mensaje));
		if(recv(sock, mensaje, sizeof(mensaje), 0) <= -1)
		{
			printf("Error al recibir la posicion\n");
			exit(1);
		}
		posicion = ntohs (*((int*)mensaje));
		
		if (posicion == 0)
		{
			return;
		}
		else if(posicion < 1 || posicion > max)
		{	
			bzero(mensaje, sizeof(mensaje));
			*((struct nocon_rp*) mensaje) = nocon_rp;			
			
			if ((send(sock, mensaje, sizeof(struct nocon_rp), 0)) <= -1) 
			{
			  printf("error al enviar la no confirmacion\n");
			}
		}
		else
		{	
			bzero(mensaje, sizeof(mensaje));
			*((struct con_rp*) mensaje) = con_rp;			
			
			if ((send(sock, mensaje, sizeof(struct con_rp), 0)) <= -1) 
			{
			  printf("error al enviar la confirmacion\n");
			}
		}
		
	}while(posicion < 1 || posicion > max);
	
	if ((recv (sock, mensaje, MAX_BUFF_SIZE, 0)) <= -1)
	{
		printf("Error en el recieve");
		exit(1);
	}
	buff = mensaje;
	mod_rule = *((rule*) buff);
	aux = chain -> first_rule;
	
	do
	{
		if(posicion == i)
		{
			aux->rule = mod_rule;
			found = true;
		}
		else
		{
			aux = aux -> next_rule;
			i += 1;
		}
	}while(!found);
	
}
void process_DELETE_msg(int sock, struct FORWARD_chain *chain)
{
	char mensaje [MAX_BUFF_SIZE];
	bzero(mensaje, sizeof(mensaje));
	struct fw_rule* aux;
	struct fw_rule* aux_next;
	bool found = false;
	int i = 1;
	int posicion;
	int max;
	
	struct del_rp del_rp;
	strcpy(del_rp.msg, "Inserta la posicion de la entrada a borrar o pulsa 0 para salir\n");
	
	struct con_rp con_rp;
	stshort(1, &con_rp.opcode);
	strcpy(con_rp.msg, "Valor valido\n");	
	
	struct nocon_rp nocon_rp;
	stshort(0, &nocon_rp.opcode);
	strcpy(nocon_rp.msg, "Valor no valido\n");
	
	max = chain->num_rules;
		
	do
	{   
	    bzero(mensaje, sizeof(mensaje));
	    *((struct del_rp*) mensaje) = del_rp;
	  
	    if ((send(sock, mensaje, sizeof(struct del_rp), 0)) <= -1) 
		{
		  printf("error al enviar mensaje de borrado\n");
	    }
		
		bzero(mensaje, sizeof(mensaje));
		if(recv(sock, mensaje, sizeof(mensaje), 0) <= -1)
		{
			printf("Error al recibir la posicion\n");
			exit(1);
		}
		posicion = ntohs (*((int*)mensaje));
		
		
		if (posicion == 0)
		{
			return;
		}
		else if(posicion < 1 || posicion > max)
		{	
			bzero(mensaje, sizeof(mensaje));
			*((struct nocon_rp*) mensaje) = nocon_rp;			
			
			if ((send(sock, mensaje, sizeof(struct nocon_rp), 0)) <= -1) 
			{
			  printf("error al enviar la no confirmacion\n");
			}
		}
		else
		{	
			bzero(mensaje, sizeof(mensaje));
			*((struct con_rp*) mensaje) = con_rp;			
			
			if ((send(sock, mensaje, sizeof(struct con_rp), 0)) <= -1) 
			{
			  printf("error al enviar la confirmacion\n");
			}
		}
		
	}while(posicion < 1 || posicion > max);
		
	printf("posicion %d \n", posicion);
	aux = chain -> first_rule;
	aux_next = chain -> first_rule;
	aux_next = aux_next -> next_rule;
	
	
	if (posicion == 1)
	{	
		
		aux->next_rule=NULL;
		chain->first_rule= aux_next;
		free(aux);
	}
		else
		{ 	

			while(!found)
			{	
				i += 1;
				if(i == posicion)
				{
					aux -> next_rule = aux_next -> next_rule;
					free(aux_next);
					found = true;
				}
				else
				{
					aux = aux -> next_rule;
					aux_next = aux_next -> next_rule;
					
				}
				
			}
			
		
		}
	chain->num_rules=chain->num_rules-1;
	}
		
void process_FLUSH_msg(int sock, struct FORWARD_chain *chain)
{
	
	int i = 0;
	int max = chain->num_rules; i++;
	struct fw_rule *aux;
    struct fw_rule *aux_next;
	
	aux = chain -> first_rule;
	chain -> first_rule = NULL;
	

	for (i = 0; i < max; i++)
	{
		aux_next = aux -> next_rule;
		aux->next_rule = NULL;
		free(aux);
		aux = aux_next;
	}

	chain->num_rules = 0;
}	

 /** 
 * Receives and process the request from a client.
 * @param the socket connected to the client.
 * @param chain the chain with the filter rules.
 * @return 1 if the user has exit the client application therefore the 
 * connection whith the client has to be closed. 0 if the user is still 
 * interacting with the client application.
 */
int process_msg(int sock, struct FORWARD_chain *chain)
{
  unsigned short op_code;
  int finish = 0;
  char input[MAX_BUFF_SIZE];
  bzero(input,sizeof(input));

  if ((recv(sock, input , MAX_BUFF_SIZE, 0)) <0)
  {
	  printf("Error al recibir los datos");
	  exit(1);
  }
  else { printf("request recieved\n"); }

  op_code = ldshort(input);

  switch(op_code)
  {
    case MSG_HELLO:
      process_HELLO_msg(sock);
      break;    
    case MSG_LIST: 
	  process_LIST_msg(sock, chain);
      break;
    case MSG_ADD:
	  process_ADD_msg(sock, input+2, chain);
      break;                              
    case MSG_CHANGE:
      process_CHANGE_msg(sock, chain);
      break;
    case MSG_DELETE:
      process_DELETE_msg(sock, chain);
      break;
    case MSG_FLUSH:
      process_FLUSH_msg(sock, chain);
      break;
    case MSG_FINISH:
      finish = 1;
      break;
    default:
      perror("Message code does not exist.\n");
  } 
  
  return finish;
}
 
int main(int argc, char *argv[])
{
  int port = getPort(argc, argv);
  int nuevo;
  struct FORWARD_chain chain;
  int finish = 0;
  int sockser, pid;
  socklen_t clientlen;
  chain.num_rules = 0;
  chain.first_rule = NULL;
  
  struct sockaddr_in client; //no se por que se hace
  clientlen = sizeof(client);
  
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = INADDR_ANY;
  

  sockser = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockser <= -1)
  {
	  printf("error en creacion de socket()\n");
	  exit(1);
  }
  else {  printf("socket creado\n");  }	
	
  if ( bind(sockser, (struct sockaddr *)&server, sizeof(server)) <= -1)
  {
		printf("Error en el bind\n");
		exit(1);
  }
  else {  printf("bind hecho\n"); }
  
  if (listen(sockser, MAX_QUEUED_CON) <= -1)
  {
		printf("error en listen()\n");
		exit(1);
  }
  else {  printf("listening\n");  }
	
  while(1) 
  {
    
    if((nuevo = accept(sockser, (struct sockaddr*)&client, &clientlen)) < 0)
	{
      printf("Error en el accept");
      exit(1);
    }
      pid=fork();
      if(pid != 0){ 
          close(nuevo);
      }
	  else
	  { close(sockser);
		do{
			finish = process_msg(nuevo, &chain);
		}while(!finish);
	  }
    close(nuevo);
  }
  return 0;
}
