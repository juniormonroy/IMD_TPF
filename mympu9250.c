#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>

/*****************************************************************************************************/
#define  DEVICE_NAME "i2c_mse"    
#define  CLASS_NAME  "i2c"        

static int    majorNumber;                  
static char   message[256] = {0};            /*Memoria para la cadena que se pasa desde userspace*/
static short  size_of_message;               /*Se utiliza para recordar el tamano de la cadena almacenada*/
static int    CountOpen = 0;               /*Cuenta el numero de veces que se abre el dispositivo*/
static struct class*  CharClass  = NULL;     /*Puntero device-driver class struct */
static struct device* CharDevice = NULL;     /*Puntero device-driver device struct*/
static struct i2c_client *modClient;

/* Prototipo de funciones para el character driver */
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static loff_t  device_lseek(struct file *file, loff_t offset, int orig);

/*Lectura de temperatura*/
char ADDRESS[] = {0x41};/*0100 0001*/


/*****************************************************************************************************/
/* Definicion de estructuras */

static const struct i2c_device_id mympu9250_i2c_id[] = {

{ "mympu9250", 0 },

{ }

};

/* Invocar a MODULE_DEVICE_TABLE(of, my_of_match_table ) para exponer el dispositivo */
MODULE_DEVICE_TABLE(i2c, mympu9250_i2c_id);

static const struct of_device_id mympu9250_of_match[] = {

    { .compatible = "mse,mympu9250" },

    { }
};


MODULE_DEVICE_TABLE(of, mympu9250_of_match);


static struct file_operations fops =
{
   .open = dev_open,

   .read = dev_read,

   .write = dev_write,

   .release = dev_release,

   .llseek = device_lseek,
};

/*****************************************************************************************************/
static int __init char_init(void){

   printk("\n### "KERN_INFO "\n ### INicializando el Char loadable kernel modules ###");

   /*Intente asignar dinamicamente un numero mayor para el dispositivo*/
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);

   if (majorNumber<0){

      printk("\n### "KERN_ALERT "Char fallo para registrar major number ###");

      return majorNumber;
   }
   printk("\n### "KERN_INFO "Char registrado correctamente con el major number %d ###", majorNumber);

   /* Registrar el device class */
   CharClass = class_create(THIS_MODULE, CLASS_NAME);

   if (IS_ERR(CharClass)){                /*Comprobacion de errores y limpieza*/

      unregister_chrdev(majorNumber, DEVICE_NAME);

      printk("\n### "KERN_ALERT "Fallo el registro del device class ###");

      return PTR_ERR(CharClass);          /*Forma correcta de devolver un error en un puntero*/
   }

   printk("\n### "KERN_INFO "Char device class registrado correctamente ###");

   /*Registro del device driver*/
   CharDevice = device_create(CharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);

   if (IS_ERR(CharDevice)){               /* Limpia si hay un error */

      class_destroy(CharClass);           /*alternativa al goto*/

      unregister_chrdev(majorNumber, DEVICE_NAME);

      printk("\n### "KERN_ALERT "Fallo al crear el device ###");

      return PTR_ERR(CharDevice);
   }

   printk("\n### "KERN_INFO "Char device class creado correctamente ###");  /*el dispositivo fue inicializado*/

   return 0;
}


static void __init char_exit(void){

   device_destroy(CharClass, MKDEV(majorNumber, 0));     /* Remueve el device */

   class_unregister(CharClass);                          /* desregistra el device class */

   class_destroy(CharClass);                             /* Remueve el device class */

   unregister_chrdev(majorNumber, DEVICE_NAME);          /* Desregistra el major number */

   printk("\n### "KERN_INFO "fin desde  el loadable kernel modules ###");
}

/*****************************************************************************************************/
static int dev_open(struct inode *inodep, struct file *filep){

   int i=0;
   int rv;
   char buf[1] = {0x6B};
   char mpu9250_output_buffer[21];
   CountOpen++;

   printk("\n### "KERN_INFO "Char Device ha sido abierto %d veces###", CountOpen);
   printk("\n### "KERN_INFO "UPDATE ###");

   /*Lectura de registros para saber que el dispositivo funciona*/
   buf[0]    = 0x3B; /*ACCEL_XOUT_H*/

   pr_info("\n### LECTURA DE REGISTROS DE PRUEBA 0x3B  ACCEL_XOUT_H ###");

   rv = i2c_master_send(modClient,buf,1); 

   pr_info("\n### Datos Recibido: %0d",rv);

   rv = i2c_master_recv(modClient,mpu9250_output_buffer,21);

 
   
   for (i = 0; i < 21; i++)
   {
       pr_info("\n### REGISTRO=0x%02X --> Valor: 0x%02X ###",buf[0]+i,mpu9250_output_buffer[i]);
   }
   return 0;
}

/*****************************************************************************************************/
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){

   int CountError = 0;
   
   int ret;
   
   /* El offset esta configurado cuando se hace la funcion de lseek */
   pr_info("\n### i2c Inicia direccion MPU9250 0x%02X ###",(char)(*offset));


   /*Seteo el Address a partir del cual quiero escribir*/
   ADDRESS[0] = (char)(*offset);

   ret = i2c_master_send(modClient,ADDRESS,1);     
   
   if(ret < 0){

      printk("\n### "KERN_INFO "i2C No se pudo configurar el registro para leer los datos desde el MPU9250 ###");
   }

   pr_info("\n### i2c Cantidad de bytes configurados para leer = %d ###",len);
   
   ret = i2c_master_recv(modClient,message,len);     
   
   if(ret != len){
   
      printk("\n### "KERN_INFO "i2C No se pudieron recibir %0d bytes desde el MPU9250 ###", len);
   }
   
   /* La idea es mandarle datos al usuario */
   CountError = copy_to_user(buffer,message,ret);
   
   if (CountError==0){            /* si no hay error, envia el dato */
   
      printk("\n### "KERN_INFO "Char Sent %d characters to the user ###", ret);
   
      return (ret=0);              /* Limpia la posicion de inicio y retorna a la posicion de inicio*/
   }
   
   else {
   
      printk("\n### "KERN_INFO "Char Failed to send %d characters to the user ###", CountError);
   
      return -EFAULT;              /*Falla de envio de datos*/
   }
}

/*****************************************************************************************************/
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){

   int ret;
   char buf[256];
   pr_info("\n ### i2c Start Reg Address MPU9250: 0x%02X ###",(char)(*offset));
   pr_info("\n ### i2c Cantidad de bytes a escribir: %0d ###",len);

   /*Cargo el address que viene en el offset*/
   buf[0] = (char)(*offset);

   /*Agarro los datos provenientes del usuario*/
   ret = copy_from_user(message,buffer,len);

   size_of_message = strlen(message);

   pr_info("\n ### i2c Bytes recibidos desde el usuario:%d ###",size_of_message);
   /*Copio el mensaje proveniente del usuario en el mensaje a enviar al usuario*/


   /*Recordar que el address que hay que poner al principio hay que ponerlo y por eso va el +1*/
   strcpy(buf+1,message);

   /*Escribo por el I2C con el address al principio*/
   ret = i2c_master_send(modClient,buf,size_of_message);
   pr_info("\n### i2c Se escribieron %d bytes a través del I2C ###",ret);

   return len;
}

static int dev_release(struct inode *inodep, struct file *filep){
      int i=0;
   int rv;
   char buf[1] = {0x6B};
   char mpu9250_output_buffer[21];
      printk(KERN_INFO "EBBChar: Device successfully closed\n");

   /*Lectura de 22 registros ,dispositivo sigue respondiendo*/
   buf[0]    = 0x3B;
   pr_info("READ 21 REGISTER FROM ADDRES 0x3B\n");
   rv = i2c_master_send(modClient,buf,1);
   rv = i2c_master_recv(modClient,mpu9250_output_buffer,21);
   pr_info("Datos Recibido: %0d\n",rv);
   for (i = 0; i < 21; i++)
   {
       pr_info("REGISTRO=0x%02X --> Valor: 0x%02X\n",buf[0]+i,mpu9250_output_buffer[i]);
   }
   return 0;
}


static loff_t device_lseek(struct file *file, loff_t offset, int orig) {
    loff_t new_pos = 0;

    printk("\n### "KERN_INFO "lseek en ejecucion mympu9250 ###");
   /*No importa la configuracion del offset siempre se configura con el valor del parametro */ 

   /*Esto es porque no tenemos ningun buffer y la idea es accederlo como un mapa de registros */
    switch(orig) {
        case 0 : /*seek set*/
            new_pos = offset;
            break;
        case 1 : /*seek cur*/
            new_pos = offset;
            break;
        case 2 : /*seek end*/
            new_pos = offset;
            break;
    }
    file->f_pos = new_pos;

    return new_pos;
}

/*****************************************************************************************************/
/*Funcion a a ejecutarse cuando se instale el kernel object*/
static int mympu9250_probe(struct i2c_client *client,const struct i2c_device_id *id){
    int i=0;
    int rv;
    char buf[1] = {0x6B};/*PWR_MGMT_1*/
    char wr_buf[2] = {0x6B,0x01};/*PWR_MGMT_1 , SELF_TEST_Y_GYRO*/
    char rd_buf[2];
    char rv_data; 
    char mpu9250_output_buffer[21];
    
    char_init();
    modClient = client;

   /*En la instanciacion de este driver se hacen algunas lecturas y escrituras spare para probar la conexion */
    pr_info("\n### PROBE:mympu9250");   
    pr_info("\n### WRITE ADDRESS 0x6B con 0x1");
    rv = i2c_master_send(client,wr_buf,2);


    pr_info("\n### i2c_master_send: 0x%02X ###",rv);
    pr_info("\n### READ ADDRESS 0x6B ###");
    rv = i2c_master_send(client,buf,1);


    rv = i2c_master_recv(client,&rv_data,1);
    pr_info("\n### i2c_master_receive: 0x%02X Numero de Datos: %0d",rv_data,rv);


    wr_buf[0] = 0x6A; /*Registro I2C USER_CTRL del mpu9250 */
    wr_buf[1] = 0x20; /*YG_OFFSET_H*/
    buf[0]    = 0x6A; /*USER_CTRL*/

    pr_info("\n### WRITE ADDRESS 0x6A con 0x20");
    rv = i2c_master_send(client,wr_buf,2);

    pr_info("\n### i2c_master_send: 0x%02X",rv);
    pr_info("\n### READ ADDRESS 0x6A ###");
    rv = i2c_master_send(client,buf,1);
    rv = i2c_master_recv(client,rd_buf,2);

    pr_info("\n### i2c_master_receive: 0x%02X 0x%02X Numero de Datos: %0d",rd_buf[0],rd_buf[1],rv);
    buf[0]    = 0x3B;

    pr_info("\n### Lectura 20 registros desde ADDRES 0x3B ###");
    rv = i2c_master_send(client,buf,1);
    rv = i2c_master_recv(client,mpu9250_output_buffer,21);

    pr_info("\n### Datos Recibido: %0d",rv);
    for (i = 0; i < 21; i++)
    {
        pr_info("\n### REGISTRO=0x%02X --> Valor: 0x%02X\n",buf[0]+i,mpu9250_output_buffer[i]);
    }
    return 0;
}

/*****************************************************************************************************/
static int mympu9250_remove(struct i2c_client *client)
{
    pr_info("\n### REMOVE mympu9250 ###");

   char_exit();

    return 0;
}

static struct i2c_driver mympu9250_i2c_driver = {
    .driver = {
        .name = "mympu9250",

        .of_match_table = mympu9250_of_match,

    },

    .probe = mympu9250_probe,
    .remove = mympu9250_remove,
    .id_table = mympu9250_i2c_id,
};

/*****************************************************************************************************/

/*Macro module_i2c_driver() para exponer el driver al kernel, pasando como argumento la estructura i2c_driver declarada antes.*/
module_i2c_driver(mympu9250_i2c_driver);

/*****************************************************************************************************/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("JUnior Monroy");
MODULE_DESCRIPTION("Driver para lectura del dispositivo MPU9250");


