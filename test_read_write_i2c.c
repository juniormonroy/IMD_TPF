#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>

#define ESCALA_TEMP        333.87f
#define OFFSET_TEMP        21.0f
#define I2C_REG_TEMP       0x41  /*0100 0001*/
#define I2C_USER_CTRL      0x6A /*0110 1010*/
#define I2C_PWR_MGMT_1     0x6B /*0110 1011*/


static char   buffer[256] = {0}; 
static int ret,fd;
static int cant_lectura,cant_escritura;
static int offset;
static char wr_buff[256] = {0};
static float temp;

/****************************************************************************************/
/************************LECTURA DEL DRIVER EN DEV **************************************/

int main (void){

printf("################### INICIO DEL PROGRAMA ###################### \n");

   fd = open("/dev/i2c_mse", O_RDWR);
      if (fd < 0){

         perror("*** FALLA AL ABRIR EL DISPOSITIVO***");

         return errno;
      }   

/****************************************************************************************/
/****************************LECTURA I2C*************************************************/
   printf("\n\n\n*** LECTURA I2C DE I2C_REG_TEMP 0x41 ***");

   lseek(fd,I2C_REG_TEMP,SEEK_SET);

   cant_lectura = 2;

   printf("\n*** SE LEERAN %d bytes***",cant_lectura);


   ret = read(fd,buffer,cant_lectura);

      if(ret < 0){

         printf("\n*** FALLO LEER EL MENSAJE DESDE EL DISPOSITIVO ***");

         close(fd);

         return -1;
      }

   printf("\n*** REGISTRO=0x%02X --> Valor: 0x%02X ***",I2C_REG_TEMP,buffer[0]);

   printf("\n*** REGISTRO=0x%02X --> Valor: 0x%02X ***",I2C_REG_TEMP+1,buffer[1]);

   temp = (float)(buffer[0]<<8);

   temp = temp + (float)buffer[1];

   temp = ((((float)temp - OFFSET_TEMP)/ESCALA_TEMP) + OFFSET_TEMP);


   printf("\n*** TEMPERATURA GRADOS :%2.2f ***",temp);

/****************************************************************************************/
/*******************************ESCRITURA I2C********************************************/
   printf("\n\n\n*** ESCRITURA I2C A I2C_USER_CTRL 0x6A ***");

   offset = I2C_USER_CTRL;

   lseek(fd,offset,SEEK_SET);

   wr_buff[0] = 0x20; /*0010 0000*/

   cant_escritura = 1;

   printf("\n*** ESCRITOS %d BYTES AL REGISTRO: 0x%02X ***",cant_escritura,I2C_USER_CTRL);

   ret = write(fd,wr_buff,cant_escritura);

   if(ret < 0){

      printf("\n*** FALLA DE ESCRITURA AL DISPOSITIVO***");

      close(fd);

      return -1;
   }

/****************************************************************************************/
/*******************************ESCRITURA I2C********************************************/
   printf("\n\n\n*** ESCRITURA I2C A I2C_PWR_MGMT_1 0x6B  ***");

   offset = I2C_PWR_MGMT_1;

   lseek(fd,offset,SEEK_SET);

   wr_buff[0] = 0x1; /*0000 0001*/

   cant_escritura = 1;

   printf("\n*** ESCRITOS %d BYTES AL REGISTRO: 0x%02X ***",cant_escritura,I2C_PWR_MGMT_1);

   ret = write(fd,wr_buff,cant_escritura);

   if(ret < 0){

      printf("\n*** FALLA DE ESCRITURA AL DISPOSITIVO ***");

      close(fd);

      return -1;
   }

/****************************************************************************************/

   printf("\n*** FIN DE LA APLICACION USUARIO ***");

   close(fd);

   

printf("\n################### FIN ######################");

return 0;
}
/****************************************************************************************/
