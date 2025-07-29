#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>

MODULE_AUTHOR("DevTITANS <devtitans@icomp.ufam.edu.br>");
MODULE_DESCRIPTION("Driver de acesso ao SmartLamp (ESP32 com Chip Serial CP2102");
MODULE_LICENSE("GPL");


#define MAX_RECV_LINE 100 // Tamanho máximo de uma linha de resposta do dispositvo USB


static struct usb_device *smartlamp_device;        // Referência para o dispositivo USB
static uint usb_in, usb_out;                       // Endereços das portas de entrada e saida da USB
static char *usb_in_buffer, *usb_out_buffer;       // Buffers de entrada e saída da USB
static int usb_max_size;                           // Tamanho máximo de uma mensagem USB

#define VENDOR_ID 0x10c4 /* Encontre o VendorID  do smartlamp */
#define PRODUCT_ID 0xea60  /* Encontre o ProductID do smartlamp */
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id); // Executado quando o dispositivo é conectado na USB
static void usb_disconnect(struct usb_interface *ifce);                           // Executado quando o dispositivo USB é desconectado da USB
static int  usb_read_serial(void);                                                   // Executado para ler a saida da porta serial

MODULE_DEVICE_TABLE(usb, id_table);
bool ignore = true;
int LDR_value = 0;

static struct usb_driver smartlamp_driver = {
    .name        = "smartlamp",     // Nome do driver
    .probe       = usb_probe,       // Executado quando o dispositivo é conectado na USB
    .disconnect  = usb_disconnect,  // Executado quando o dispositivo é desconectado na USB
    .id_table    = id_table,        // Tabela com o VendorID e ProductID do dispositivo
};

module_usb_driver(smartlamp_driver);

// Executado quando o dispositivo é conectado na USB
static int usb_probe(struct usb_interface *interface, const struct usb_device_id *id) {
    struct usb_endpoint_descriptor *usb_endpoint_in, *usb_endpoint_out;

    printk(KERN_INFO "SmartLamp: Dispositivo conectado ...\n");

    // Detecta portas e aloca buffers de entrada e saída de dados na USB
    smartlamp_device = interface_to_usbdev(interface);
    ignore =  usb_find_common_endpoints(interface->cur_altsetting, &usb_endpoint_in, &usb_endpoint_out, NULL, NULL);  // AQUI
    usb_max_size = usb_endpoint_maxp(usb_endpoint_in);
    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    LDR_value = usb_read_serial();

    printk("LDR Value: %d\n", LDR_value);

    return 0;
}

// Executado quando o dispositivo USB é desconectado da USB
static void usb_disconnect(struct usb_interface *interface) {
    printk(KERN_INFO "SmartLamp: Dispositivo desconectado.\n");
    kfree(usb_in_buffer);                   // Desaloca buffers
    kfree(usb_out_buffer);
}

static int usb_read_serial() {
    int ret, actual_size;
    int retries = 10;

     // Envia o comando para o firmware
    strcpy(usb_out_buffer, "GET_LDR\n"); // ou "GET_LDR\r\n" dependendo do seu firmware
    ret = usb_bulk_msg(smartlamp_device,
                       usb_sndbulkpipe(smartlamp_device, usb_out),
                       usb_out_buffer,
                       strlen(usb_out_buffer),
                       &actual_size,
                       1000);
    if (ret) {
        printk(KERN_ERR "SmartLamp: Erro ao enviar comando GET_LDR. Codigo: %d\n", ret);
        return -EIO;
    }
    

    while (retries > 0) {

        ret = usb_bulk_msg(smartlamp_device,
                           usb_rcvbulkpipe(smartlamp_device, usb_in),
                           usb_in_buffer,
                           min(usb_max_size, MAX_RECV_LINE),
                           &actual_size,
                           1000);
        if (ret) {
            printk(KERN_ERR "SmartLamp: Erro ao ler dados da USB (tentativa %d). Codigo: %d\n", 10 - retries + 1, ret);
            retries--;
            continue;
        }

        // Garante que é uma string terminada
        if (actual_size < usb_max_size)
            usb_in_buffer[actual_size] = '\0';
        else
            usb_in_buffer[usb_max_size - 1] = '\0';

        printk(KERN_INFO "SmartLamp: Resposta recebida: %s\n", usb_in_buffer);

        // Localiza último espaço (antes do número)
        char *token = strrchr(usb_in_buffer, ' ');
        if (!token) {
            printk(KERN_ERR "SmartLamp: Resposta inválida.\n");
            return -EINVAL;
        }

        int value;
        if (kstrtoint(token + 1, 10, &value) != 0) {
            printk(KERN_ERR "SmartLamp: Erro ao converter valor LDR.\n");
            return -EINVAL;
        }

        LDR_value = value;
        return value;
    }

    printk(KERN_ERR "SmartLamp: Falha ao ler o valor LDR após múltiplas tentativas.\n");
    return -1;
}
