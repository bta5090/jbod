#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

// Defined constant to make use of full packet byte width convenient
#define FULL_PACKET_SIZE 264


// The client socket descriptor for the connection to the server
int cli_sd = -1;


// Attempts to read n (len) bytes from fd and returns true on success and false on failure. 
// While this function is called in my rec_packet() function, I did not implement it due to difficulty figuring out the logic.
// May need to call the system call "read" multiple times to reach the given size len. 
static bool nread(int fd, int len, uint8_t *buf) {
  
  //int i = 0;
  //int data_Read = 0;


  return false;

}


/* Attempts to write n bytes to fd and returns true on success and false on failure 
// This function like the one above was not implemented due to still figuring out the logic for multiple system calls through buffer.
// May call the system call "write" multiple times to reach the size len. */
static bool nwrite(int fd, int len, uint8_t *buf) {
  
  return false;

}



// Through this function call the client attempts to receive a packet from sd (a response from the server.) after the client first sends a jbod operation call via a request message to the server. It returns true on success and false on failure. The values of the parameters (including op, ret, block) will be returned to the caller of this function

// op - the address to store the jbod "opcode"  
// ret - the address to store the return value of the server side calling the corresponding jbod_operation function.
// block - holds the received block content if existing (e.g., when the op command is JBOD_READ_BLOCK). 

static bool recv_packet(int sd, uint32_t *op, uint16_t *ret, uint8_t *block) {

  // Initialise variables to store extracted fields from response message
  u_int16_t recieve_len;
  // Pointer variable that will store return value of server-side jbod operations
  int16_t *recieve_ret = NULL;
  u_int32_t recieve_op;
  // packet to store protocol message that will be extracted
  u_int8_t packet[FULL_PACKET_SIZE] = {0};

  // Different fields in protocol message extracted using their associated byte widths
  memcpy(&recieve_len, packet, sizeof(recieve_len));
  memcpy(&recieve_op, packet + 2, sizeof(recieve_op));
  memcpy(&recieve_len, packet + 6, sizeof(recieve_ret));

  // Decode fields back from network format
  recieve_len = ntohs(recieve_len);
  recieve_op = ntohs(*op);
  *recieve_ret = ntohs(*ret);


  return true;

}




// The client attempts to send a jbod request packet to sd (server socket) and returns true on success and false on failure. 
// op - the opcode. 
// block- when the command is JBOD_WRITE_BLOCK, the block will contain data to write to the server jbod system; otherwise it is NULL.
static bool send_packet(int sd, uint32_t op, uint8_t *block) {
  
  // Accounts for when block is not empty/NULL
  if (block != NULL)
  {
    // length must be extended by block size due to non-empty block
    u_int16_t send_len = HEADER_LEN + JBOD_BLOCK_SIZE;

    u_int16_t ret = 0;
    // Packet array container for opcode and buffer and ret and length
    u_int8_t packet[FULL_PACKET_SIZE] = {0};

    // NBO formatting before sending request message as JBOD protocol
    send_len = htons(send_len);
    op = htonl(op);
    ret = htons(ret);

    // Copied into packet array of size 264 using respective byte widths 
    memcpy(packet + 0, &send_len, sizeof(send_len));
    memcpy(packet + 2, &op, sizeof(op));
    memcpy(packet + 6, &ret, sizeof(ret));

    // If block is not NULL it too is added to the packet request message.
    if (block != NULL)
    {
      memcpy(packet + 8, block, JBOD_BLOCK_SIZE);
    }
    
    return true;

  }
  else
  {
    u_int16_t send_len = HEADER_LEN;

    u_int16_t ret = 0;
    u_int8_t packet[FULL_PACKET_SIZE] = {0};

    send_len = htons(send_len);
    op = htonl(op);
    ret = htons(ret);

    memcpy(packet + 0, &send_len, sizeof(send_len));
    memcpy(packet + 2, &op, sizeof(op));
    memcpy(packet + 6, &ret, sizeof(ret));

    if (block != NULL)
    {
      memcpy(packet + 8, block, JBOD_BLOCK_SIZE);
    }

    return true;

  }


}




// Connects to server using socket and sets the global cli_sd variable to the socket. If connection is good, it returns true and false if not.
bool jbod_connect(const char *ip, uint16_t port) {
  
  // File handle created for connection with server
  int cli_sd = socket(AF_INET, SOCK_STREAM, 0);

  // Checks to make sure file descriptor is not -1 through some error
  if (cli_sd == -1)
  {
    return false;
  }
  else if (cli_sd != -1)
  {
    // Address and Port and IP of server specified
    const char *servIp = ip;
    unsigned short servPort = port;
    // Struct created to hold address
    struct sockaddr_in connection;

    // Address struct fields for socket defined correctly using Network Byte Order formatting
    connection.sin_family = AF_INET;
    connection.sin_port = htons(servPort);
    // Recieving connection at local host so address doesn't need specification
    connection.sin_addr.s_addr = htonl(INADDR_ANY);

    // IP address converted into UNIX structure
    int x = inet_aton(servIp, &(connection.sin_addr));
    if (x == 0)
    {
      return false;
    }

    // Connects socket file descriptor to specified server address and port
    int r = connect(cli_sd, (const struct sockaddr *) &connection, sizeof(connection));
    if (r == -1)
    {
      return false;
    }

    return true;
    
  }
  
  return false;

}




/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {

  // Socket disconnected using close system call
  close(cli_sd);

  // global socket descriptor re-initialised to -1 (no socket/connection established)
  cli_sd = -1;

}



// Function sends the JBOD operation to the server using send_packet() and receives using the recv_packet function and then processes the response by extracting the ret value from the server-side jbod operations. 
// The meaning of each parameter is the same as in the original jbod_operation function return: 0 means success, -1 means failure.
int jbod_client_operation(uint32_t op, uint8_t *block) {
  
  // Checks that socket is already established or else fails
  if (cli_sd == -1)
  {
    return -1;
  }
  
  // Sends request message to server using above arguments
  bool request = send_packet(cli_sd, op, block);
  if (request == false)
  {
    return -1;
  }

  // Initialise variables to hold the returned opcode and ret values from response message below.
  uint32_t newOp = 0x00;
  uint16_t *ret = NULL;

  // Retrieves response from server and decodes protocol message back from network format
  bool response = recv_packet(cli_sd, &newOp, ret, block);
  if (response == false)
  {
    return -1;
  }
  // If the returned op is not the same as the request message or the jbod operation in the server failed, function returns -1
  else if (newOp != op || *ret == -1)
  {
    return -1;
  } 
  
  return 0;

}