
#include "lpc_types.h"
#include "usb.h"
#include "cdc.h"
#include "usbcfg.h"
#include "usbdesc.h"
#include "config.h"


/* USB Standard Device Descriptor */
const uint8_t USB_DeviceDescriptor[] =
{
    USB_DEVICE_DESC_SIZE,              /* bLength */
    USB_DEVICE_DESCRIPTOR_TYPE,        /* bDescriptorType */
    WBVAL(0x0200), /* 2.0 */           /* bcdUSB */
    USB_DEVICE_CLASS_COMMUNICATIONS,   /* bDeviceClass CDC*/
    0x00,                              /* bDeviceSubClass */
    0x00,                              /* bDeviceProtocol */
    USB_MAX_PACKET0,                   /* bMaxPacketSize0 */
    WBVAL(USB_VID),                    /* idVendor */
    WBVAL(USB_PID),                    /* idProduct */
    WBVAL(0x0100), /* 1.00 */          /* bcdDevice */
    0x01,                              /* iManufacturer */
    0x02,                              /* iProduct */
    0x00,                              /* iSerialNumber */
    0x01                               /* bNumConfigurations: one possible configuration*/
};

/* USB Configuration Descriptor */
/*   All Descriptors (Configuration, Interface, Endpoint, Class, Vendor */
const uint8_t USB_ConfigDescriptor[] =
{
    /* Configuration 1 */
    USB_CONFIGUARTION_DESC_SIZE,            /* bLength */
    USB_CONFIGURATION_DESCRIPTOR_TYPE,      /* bDescriptorType */
    WBVAL(                                  /* wTotalLength */
        1 * USB_CONFIGUARTION_DESC_SIZE +
        1 * USB_INTERFACE_DESC_SIZE     +   /* communication interface */
        0x0013                          +   /* CDC functions */
        1 * USB_ENDPOINT_DESC_SIZE      +   /* interrupt endpoint */
        1 * USB_INTERFACE_DESC_SIZE     +   /* data interface */
        2 * USB_ENDPOINT_DESC_SIZE          /* bulk endpoints */
    ),
    0x02,                              /* bNumInterfaces */
    0x01,                              /* bConfigurationValue: 0x01 is used to select this configuration */
    0x00,                              /* iConfiguration: no string to describe this configuration */
    USB_CONFIG_BUS_POWERED /*|*/       /* bmAttributes */
    /*USB_CONFIG_REMOTE_WAKEUP*/,
    USB_CONFIG_POWER_MA(100),          /* bMaxPower, device power consumption is 100 mA */

    /* Interface 0, Alternate Setting 0, Communication class interface descriptor */
    USB_INTERFACE_DESC_SIZE,           /* bLength */
    USB_INTERFACE_DESCRIPTOR_TYPE,     /* bDescriptorType */
    USB_CDC_CIF_NUM,                   /* bInterfaceNumber: Number of Interface */
    0x00,                              /* bAlternateSetting: Alternate setting */
    0x01,                              /* bNumEndpoints: One endpoint used */
    CDC_COMMUNICATION_INTERFACE_CLASS, /* bInterfaceClass: Communication Interface Class */
    CDC_ABSTRACT_CONTROL_MODEL,        /* bInterfaceSubClass: Abstract Control Model */
    0x00,                              /* bInterfaceProtocol: no protocol used */
    0x00,                              /* iInterface: no descriptor */
    /*Header Functional Descriptor*/
    0x05,                              /* bLength: Endpoint Descriptor size */
    CDC_CS_INTERFACE,                  /* bDescriptorType: CS_INTERFACE */
    CDC_HEADER,                        /* bDescriptorSubtype: Header Func Desc */
    WBVAL(CDC_V1_10), /* 1.10 */       /* bcdCDC */
    /*Call Management Functional Descriptor*/
    0x05,                              /* bFunctionLength */
    CDC_CS_INTERFACE,                  /* bDescriptorType: CS_INTERFACE */
    CDC_CALL_MANAGEMENT,               /* bDescriptorSubtype: Call Management Func Desc */
    0x01,                              /* bmCapabilities: device handles call management */
    0x01,                              /* bDataInterface: CDC data IF ID */
    /*Abstract Control Management Functional Descriptor*/
    0x04,                              /* bFunctionLength */
    CDC_CS_INTERFACE,                  /* bDescriptorType: CS_INTERFACE */
    CDC_ABSTRACT_CONTROL_MANAGEMENT,   /* bDescriptorSubtype: Abstract Control Management desc */
    0x02,                              /* bmCapabilities: SET_LINE_CODING, GET_LINE_CODING, SET_CONTROL_LINE_STATE supported */
    /*Union Functional Descriptor*/
    0x05,                              /* bFunctionLength */
    CDC_CS_INTERFACE,                  /* bDescriptorType: CS_INTERFACE */
    CDC_UNION,                         /* bDescriptorSubtype: Union func desc */
    USB_CDC_CIF_NUM,                   /* bMasterInterface: Communication class interface is master */
    USB_CDC_DIF_NUM,                   /* bSlaveInterface0: Data class interface is slave 0 */
    /*Endpoint 1 Descriptor*/            /* event notification (optional) */
    USB_ENDPOINT_DESC_SIZE,            /* bLength */
    USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType */
    USB_ENDPOINT_IN(1),                /* bEndpointAddress */
    USB_ENDPOINT_TYPE_INTERRUPT,       /* bmAttributes */
    WBVAL(0x0010),                     /* wMaxPacketSize */
    0x02,          /* 2ms */           /* bInterval */

    /* Interface 1, Alternate Setting 0, Data class interface descriptor*/
    USB_INTERFACE_DESC_SIZE,           /* bLength */
    USB_INTERFACE_DESCRIPTOR_TYPE,     /* bDescriptorType */
    USB_CDC_DIF_NUM,                   /* bInterfaceNumber: Number of Interface */
    0x00,                              /* bAlternateSetting: no alternate setting */
    0x02,                              /* bNumEndpoints: two endpoints used */
    CDC_DATA_INTERFACE_CLASS,          /* bInterfaceClass: Data Interface Class */
    0x00,                              /* bInterfaceSubClass: no subclass available */
    0x00,                              /* bInterfaceProtocol: no protocol used */
    0x00,                              /* iInterface: no descriptor */
    /* Endpoint, EP2 Bulk Out */
    USB_ENDPOINT_DESC_SIZE,            /* bLength */
    USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType */
    USB_ENDPOINT_OUT(2),               /* bEndpointAddress */
    USB_ENDPOINT_TYPE_BULK,            /* bmAttributes */
    WBVAL(USB_CDC_BUFSIZE),            /* wMaxPacketSize */
    0x00,                              /* bInterval: ignore for Bulk transfer */
    /* Endpoint, EP2 Bulk In */
    USB_ENDPOINT_DESC_SIZE,            /* bLength */
    USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType */
    USB_ENDPOINT_IN(2),                /* bEndpointAddress */
    USB_ENDPOINT_TYPE_BULK,            /* bmAttributes */
    WBVAL(USB_CDC_BUFSIZE),            /* wMaxPacketSize */
    0x00,                              /* bInterval: ignore for Bulk transfer */
    /* Terminator */
    0                                  /* bLength */
};


/* USB String Descriptor (optional) */
const uint8_t USB_StringDescriptor[] =
{
    /* Index 0x00: LANGID Codes */
    0x04,                              /* bLength */
    USB_STRING_DESCRIPTOR_TYPE,        /* bDescriptorType */
    WBVAL(0x0409), /* US English */    /* wLANGID */
    /* Index 0x01: Manufacturer */
    (9 * 2 + 2),                        /* bLength (9 Char + Type + lenght) */
    USB_STRING_DESCRIPTOR_TYPE,        /* bDescriptorType */
    WBVAL('A'),
    WBVAL('G'),
    WBVAL('R'),
    WBVAL(' '),
    WBVAL('A'),
    WBVAL('U'),
    WBVAL('D'),
    WBVAL('I'),
    WBVAL('O'),
    /* Index 0x02: Product */
    (14 * 2 + 2),                      /* bLength (14 Char + Type + lenght) */
    USB_STRING_DESCRIPTOR_TYPE,        /* bDescriptorType */
    WBVAL('M'),
    WBVAL('O'),
    WBVAL('D'),
    WBVAL(' '),
    WBVAL('C'),
    WBVAL('O'),
    WBVAL('N'),
    WBVAL('T'),
    WBVAL('R'),
    WBVAL('O'),
    WBVAL('L'),
    WBVAL('L'),
    WBVAL('E'),
    WBVAL('R'),
};
