#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <wiringPi.h>
#include <wiringSerial.h>

#define SIZE 256
#define STDIN 0
#define BAUD 115200

// bluetooth
void setup_rfcomm(uint32_t *svc_uuid_int);
void accept_rfcomm();
sdp_session_t *register_service(uint8_t rfcomm_port, uint32_t *svc_uuid_int);
void *read_rfcomm(void* buff_rfcomm1);
void *write_rfcomm(void* buffer1);
void close_rfcomm();

// TCP/IP 
void setup_ip(int port_no);
void accept_ip();
void *read_ip(void* buff_ip1);
void *write_ip(void* buffer1);
void close_ip();

// Serial
void setup_serial(int baud, char *device);
void read_serial(char *buff_serial);
void write_serial(char *buffer);
void close_serial();

int fd_ip_server, fd_rfcomm_server;
int fd_ip=-1, fd_rfcomm=-1, fd_serial=-1;
int port_num=5000;

fd_set readfds;
uint32_t svc_uuid_int[] = { 0x01110000, 0x00100000, 0x80000080, 0xFB349B5F };


int main(int argc, char *argv[]){
	char buff_ip[SIZE] = "", buff_rfcomm[SIZE] = "", buff_serial[SIZE] = "";
	char buffer[20] = "";
    char temp_buff[SIZE] = "";
	char device[20] = "/dev/ttyACM0";
	
	struct timeval tv;

	tv.tv_sec = 2;
	tv.tv_usec = 0;

	FD_ZERO(&readfds);
	
	printf("Program start up!\n");
	printf("%s\n",device);
	
	setup_serial(BAUD, device);
	setup_rfcomm(svc_uuid_int);
	setup_ip(port_num);
    
	bzero(buff_serial,sizeof(buff_serial));
	fflush(stdin);
	
	FD_SET(STDIN, &readfds);
	fd_set readfds_temp;
	
	while(1) {

		readfds_temp = readfds;
		select(FD_SETSIZE, &readfds_temp, NULL, NULL, &tv);

		if (FD_ISSET(STDIN, &readfds_temp)){
           
			scanf("%s", buffer);
			//fgets(buffer, sizeof(buffer),stdin);
            char *pos;
			if ((pos=strchr(buffer,'\n'))!=NULL)
                *pos ='\0';
            if(strcmp(buffer, "quit")==0){
				break;
			}
            if (buffer[0] == '/' && buffer[2]!='\0')
            {
                if (buffer[1] == 'b')
                {
                    write_rfcomm(&buffer[2]);
                }
                if (buffer[1] == 't')
                {
                    write_ip(&buffer[2]);
                }
                if (buffer[1] == 's' && buffer[2])
                {
                    strcat(buffer,"\n");
                    write_serial(&buffer[2]);
                }
                bzero(buffer,sizeof(buffer));
            }
		}
		
		if (FD_ISSET(fd_rfcomm_server, &readfds_temp)){
			accept_rfcomm();
		}

		if (FD_ISSET(fd_ip_server, &readfds_temp)){
			accept_ip();
		}
		
		if (fd_rfcomm > 0 && fd_ip > 0 && fd_serial > 0){
			if (FD_ISSET(fd_rfcomm, &readfds_temp)){
				read_rfcomm(buff_rfcomm);
                strcpy(temp_buff,buff_rfcomm);
                strcat(temp_buff,"\n");
				write_ip(buff_rfcomm);
                write_serial(temp_buff);
                bzero(temp_buff,sizeof(buff_rfcomm));
				bzero(buff_rfcomm,sizeof(buff_rfcomm));
			}
			
			if (FD_ISSET(fd_serial, &readfds_temp)){
				read_serial(buff_serial);
				write_ip(buff_serial);
				bzero(buff_serial,sizeof(buff_serial));
			}
			
			if (FD_ISSET(fd_ip, &readfds_temp)){
				read_ip(buff_ip);
				if(buff_ip[0] == 'a') {
					write_rfcomm(buff_ip);
					bzero(buff_ip,sizeof(buff_ip));
				} else {
                    strcat(buff_ip,"\n");
					write_serial(buff_ip);
					bzero(buff_ip,sizeof(buff_ip));
				}
			}
		}
	}
	
	close_ip();
	close_rfcomm();
	close_serial();
	printf("Programm terminating...\n");
}

void setup_rfcomm(uint32_t *svc_uuid_int){
	struct sockaddr_rc addr_server = { 0 };
	int port;
    
    // m_bt_dev_id is the first available bluetooth adapter
    int m_bt_dev_id = hci_get_route(NULL);
    if (m_bt_dev_id <0)
    {
        perror("No available Bluetooth adapter found");
    }
	// allocate bluetooth rfcomm socket
	fd_rfcomm_server = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

	if(fd_rfcomm_server < 0){
		perror("ERROR opening bluetooth socket.\n");
	} else {
		addr_server.rc_family = AF_BLUETOOTH;
		addr_server.rc_bdaddr = *BDADDR_ANY;
		for(port = 1; port < 30; port++){
			addr_server.rc_channel = port;
			if(bind(fd_rfcomm_server, 
                    (struct sockaddr *)&addr_server, 
                    sizeof(addr_server)) == 0)
				break;
		}
		sdp_session_t* session = register_service((uint8_t)port, svc_uuid_int);
		
		FD_SET(fd_rfcomm_server, &readfds);
		listen(fd_rfcomm_server, 1);
		printf("Listening to channel %d for bluetooth connection.\n", port);
	}	
}

void accept_rfcomm(){
	struct sockaddr_rc addr_client = { 0 };
	socklen_t opt = sizeof(addr_client);
	char mac_client[256] = "";
	int fd_temp;

	if((fd_temp= accept(fd_rfcomm_server, (struct sockaddr *)&addr_client, &opt)) >= 0){
		fd_rfcomm = fd_temp;
		FD_SET(fd_rfcomm, &readfds);
		ba2str(&addr_client.rc_bdaddr, mac_client);
		printf("Accept bluetooth connection from %s\n", mac_client);
	}
}

void *read_rfcomm(void* buff_rfcomm1){
	char* buff_rfcomm = (char*)buff_rfcomm1;
	int read_bluetooth;
	read_bluetooth = read(fd_rfcomm,buff_rfcomm,SIZE);
	if(read_bluetooth>0){
		printf("Receive from Bluetooth: %s\n", buff_rfcomm);
	}
	else {
		printf("Disconnected from BT.\n");
		//close(fd_rfcomm);
        close_rfcomm();     // close rfcomm to allow reconnections
		FD_CLR(fd_rfcomm, &readfds);
		fd_rfcomm = 0;
	}
}

void *write_rfcomm(void* buffer1){
	char* buffer = (char*)buffer1;
	int write_bluetooth;
    if(strlen(buffer) > 0 ){
        write_bluetooth = write(fd_rfcomm, buffer, strlen(buffer));
        if(write_bluetooth > 0){
                printf("Write to Bluetooth: %s\n", buffer);
                bzero(buffer,strlen(buffer));
        }
        else {
            printf("Disconnected from BT.\n");
            close_rfcomm();     // close rfcomm to allow reconnections
            FD_CLR(fd_rfcomm, &readfds);
            fd_rfcomm = 0;
            setup_rfcomm(svc_uuid_int); //re-setup rfcomm
        }
	}
}

// close socket
void close_rfcomm(){
    if (fd_rfcomm)
        close(fd_rfcomm);
    if (fd_rfcomm_server)
        close(fd_rfcomm_server);
	printf("Closed Bluetooth connection.\n");
}

void setup_ip(int port_no){
	struct sockaddr_in addr_server = { 0 };
    int iSetOptions = 1;
	// create server socket
	fd_ip_server = socket(AF_INET, SOCK_STREAM, 0);
	
	if(fd_ip_server < 0){
		perror("ERROR opening ip socket.\n");
	} else {
        if (setsockopt(fd_ip_server,SOL_SOCKET,SO_REUSEADDR,&iSetOptions,sizeof(int)) == -1)
        {
            perror("ERROR unable to set socket options.\n");
        }
        addr_server.sin_family = AF_INET;
        addr_server.sin_addr.s_addr = INADDR_ANY;
        addr_server.sin_port = htons(port_no);
        if(bind(fd_ip_server, (struct sockaddr *) &addr_server, sizeof(addr_server)) < 0) {
            perror("ERROR on binding.\n");
        }
            
        else{
            listen(fd_ip_server,5);
            FD_SET(fd_ip_server, &readfds);
            printf("Listen to port 5000 for ip socket connection.\n", port_no);
        }
    }
}

void accept_ip(){
    // waiting for client connection
	struct sockaddr_in addr_client = { 0 };
	int len_client = sizeof(addr_client);
	int fd_temp;
	if((fd_temp = accept(fd_ip_server, 
                        (struct sockaddr *) &addr_client, 
                        &len_client)) >= 0)
    {
        fd_ip = fd_temp;
        FD_SET(fd_ip, &readfds);
        printf("Accept ip socket connection from PC\n");
    }
}

void *read_ip(void* buff_ip1){
	char *buff_ip = (char *)buff_ip1;
	int read_ip;
	read_ip = read(fd_ip,buff_ip,SIZE);
	if(read_ip>0) {
		printf("Receive from PC: %s\n", buff_ip);
	}
	else { 
		printf("Disconnected from PC.\n");
		close(fd_ip);
		FD_CLR(fd_ip, &readfds);
		fd_ip = -1;
	}
}

void *write_ip(void* buffer1){
	char *buffer = (char *)buffer1;
	int write_ip;
	if(strlen(buffer)>0) {
		strcat(buffer, "\n");
		write_ip = write(fd_ip, buffer, strlen(buffer));
        if (write_ip > 0) {
                printf("Write to PC: %s", buffer);
                bzero(buffer,strlen(buffer));
        }
        else {
            printf("Disconnected from PC.\n");
            close(fd_ip);
            FD_CLR(fd_ip, &readfds);
            fd_ip = -1;
            setup_ip(port_num);
        }
	}
}

void close_ip(){
	if (fd_ip)
        close(fd_ip);
    if (fd_ip_server)
        close(fd_ip_server);
	printf("Close ip socket connection.\n");
}

sdp_session_t *register_service(uint8_t rfcomm_port, uint32_t *svc_uuid_int){
	const char *service_name = "Bluetooth Insecure";
	const char *service_dsc = "For bluetooth connection";
	const char *service_prov = "Bluetooth";

	uuid_t svc_uuid,
			svc_class_uuid,
			root_uuid, 
			l2cap_uuid, 
			rfcomm_uuid;
	sdp_list_t *svc_class_list = 0,
			*profile_list = 0,
			*root_list = 0,
			*l2cap_list = 0,
			*rfcomm_list = 0,
			*proto_list = 0,
			*access_proto_list = 0;

	sdp_data_t *channel = 0;
	sdp_profile_desc_t profile;

	sdp_record_t record = { 0 };
	sdp_session_t *session = 0;

	sdp_uuid128_create(&svc_uuid, &svc_uuid_int);
	sdp_set_service_id(&record, svc_uuid);

	sdp_uuid16_create(&svc_class_uuid, SERIAL_PORT_SVCLASS_ID);
	svc_class_list = sdp_list_append(0, &svc_class_uuid);
	sdp_set_service_classes(&record, svc_class_list);

	sdp_uuid16_create(&profile.uuid, SERIAL_PORT_PROFILE_ID);
	profile.version = 0x0100;
	profile_list = sdp_list_append(0, &profile);
	sdp_set_profile_descs(&record, profile_list);

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root_list = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(&record, root_list);

	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	l2cap_list = sdp_list_append(0, &l2cap_uuid);
	proto_list = sdp_list_append(0, l2cap_list);

	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	channel = sdp_data_alloc(SDP_UINT8, &rfcomm_port);
	rfcomm_list = sdp_list_append(0, &rfcomm_uuid);
	sdp_list_append(rfcomm_list, channel);
	sdp_list_append(proto_list, rfcomm_list);

	access_proto_list = sdp_list_append(0, proto_list);
	sdp_set_access_protos(&record, access_proto_list);

	sdp_set_info_attr(&record, service_name, service_prov, service_dsc);

	session = sdp_connect(BDADDR_ANY, BDADDR_LOCAL, 0);
	sdp_record_register(session, &record, 0);

	sdp_data_free(channel);
	sdp_list_free(l2cap_list, 0);
	sdp_list_free(rfcomm_list, 0);
	sdp_list_free(root_list, 0);
	sdp_list_free(access_proto_list, 0);
	sdp_list_free(proto_list, 0);
	sdp_list_free(svc_class_list, 0);
	sdp_list_free(profile_list, 0);

	return session;
}

void setup_serial(int baud, char *device){
	fd_serial = serialOpen(device, baud);
	if(fd_serial < 0){
		perror("ERROR opening serial device.\n");
	} else {
		FD_SET(fd_serial, &readfds);
		printf("Serial port connection established.\n");
	}
}

void read_serial(char *buff_serial){
	int index = 0;
	char newChar;
    
    newChar = serialGetchar(fd_serial);
    if(newChar != 'X') return;
    buff_serial[0] = newChar;
    index++;
    
	while(1){
		if(serialDataAvail(fd_serial)){
			newChar = serialGetchar(fd_serial);
			if(newChar == '\n'){
				buff_serial[index] = '\0';
				printf("Receive from Serial:%s\n",buff_serial);
				fflush(stdout);
				break;
			} else {
				buff_serial[index] = newChar;
				index ++;
			}
		}
	}
}

void write_serial(char *buffer){
	if(strlen(buffer)>0 && buffer[0]!='\n'){
		serialPuts(fd_serial, buffer);
		printf("Write to Serial: %s\n", buffer);
	}
}

void close_serial(){
    if (fd_serial)
        serialClose(fd_serial);
	printf("Close serial port.\n");
}
