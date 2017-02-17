#include <algorithm>

#include "usb/standard_requests.h"
#include "usb/dwc_otg_device.h"
#include "usb/descriptors.h"
#include "print.h"


void StandardRequests::on_set_configuration(uint8_t configuration)
{

}


SetupResult StandardRequests::handle_ctrl_setup_stage()
{
    // get descriptor
    if (get_setup_pkt().bmRequestType == (RECIPIENT_DEVICE | ENDPOINT_IN) &&
        get_setup_pkt().bRequest == GET_DESCRIPTOR
    ) {
        print("get descriptor\n");
        return send_descriptor();
    }

    // set address
    if (get_setup_pkt().bmRequestType == (RECIPIENT_DEVICE | ENDPOINT_OUT) &&
        get_setup_pkt().bRequest == SET_ADDRESS
    ) {
        print("set address {}\n", get_setup_pkt().wValue);

        device->set_address(get_setup_pkt().wValue);

        device->transmit_zlp(&get_ep0());  // status stage
        return SetupResult::NO_DATA_STAGE;
    }

    // set configuration
    // TODO support multiple configurations?
    if (get_setup_pkt().bmRequestType == (RECIPIENT_DEVICE | ENDPOINT_OUT) &&
        get_setup_pkt().bRequest == SET_CONFIGURATION
    ) {
        print("set configuration {}\n", get_setup_pkt().wValue & 0xFF);

        if (device->set_configuration(get_setup_pkt().wValue & 0xFF)) {
            device->transmit_zlp(&get_ep0());  // status stage TODO make lower level do this?
            return SetupResult::NO_DATA_STAGE;
        }

        return SetupResult::STALL;
    }

    // set interface
    if(get_setup_pkt().bmRequestType == (RECIPIENT_INTERFACE | ENDPOINT_OUT) &&
       get_setup_pkt().bRequest == SET_INTERFACE
    ) {
        print("set interface\n");

        device->transmit_zlp(&get_ep0());  // status stage
        return SetupResult::NO_DATA_STAGE;
    }

    // get device status
    // TODO bit 0 = self-powered, bit 1 = remote wkup enabled
    if (get_setup_pkt().bmRequestType == (RECIPIENT_DEVICE | ENDPOINT_IN) &&
        get_setup_pkt().bRequest == GET_STATUS
    ) {
        print("get status\n");

        static unsigned char const data[] = {0x00, 0x00};
        get_ep0().init_transfer(data, sizeof(data));
        device->start_in_transfer(&get_ep0());

        return SetupResult::DATA_STAGE;
    }

    print("unknown\n");
    return SetupResult::UNHANDLED;
}


DataResult StandardRequests::handle_ctrl_in_data_stage()
{
    return DataResult::UNHANDLED;
}


DataResult StandardRequests::handle_ctrl_out_data_stage()
{
    return DataResult::UNHANDLED;
}


void StandardRequests::handle_ctrl_status_stage()
{

}


SetupResult StandardRequests::send_descriptor()
{
    uint8_t const descriptor_type = get_setup_pkt().wValue >> 8;
    uint8_t const descriptor_index = get_setup_pkt().wValue & 0xFF;
    uint16_t const descriptor_lang_id = get_setup_pkt().wIndex;

    print("send_descriptor() type=0x{:x} index={} langid=0x{:x}: ",
        descriptor_type, descriptor_index, descriptor_lang_id);

    unsigned char const* descriptor_buf;
    uint16_t descriptor_size;

    switch (descriptor_type) {
    case DESCRIPTOR_DEVICE:
        print("device\n");
        descriptor_buf = reinterpret_cast<unsigned char const*>(&device_descriptor);
        descriptor_size = device_descriptor.bLength;  //sizeof(device_descriptor);
        break;

    case DESCRIPTOR_CONFIGURATION:
        print("config\n");
        descriptor_buf = reinterpret_cast<unsigned char const*>(&config_descriptor);
        descriptor_size = config_descriptor.wTotalLength;  //sizeof(config_descriptor);
        break;

    case DESCRIPTOR_STRING:
        print("string\n");
        if (descriptor_index == 0) {
            descriptor_buf = reinterpret_cast<unsigned char const*>(&lang_id_descriptor);
            descriptor_size = lang_id_descriptor.bLength;
//        } else if (descriptor_index == 0xEE) {
//            descriptor_buf = msft_string_descriptor;
//            descriptor_size = msft_string_descriptor[0];
        } else {
            //if (descriptor_index > sizeof(string_descriptors) / sizeof(string_descriptors[0])) {
            if (descriptor_index > string_descriptors_len) {
                print("index too large!\n");
                return SetupResult::STALL;
            }

            descriptor_buf = string_descriptors[descriptor_index - 1];
            descriptor_size = descriptor_buf[0];
        }
        break;

    default:
        print("unknown, stall\n");
        return SetupResult::STALL;
    }

    // host can request more or fewer bytes than the actual descriptor size
    uint16_t length = std::min(descriptor_size, get_setup_pkt().wLength);

    get_ep0().init_transfer(descriptor_buf, length);
    device->start_in_transfer(&get_ep0());

    return SetupResult::DATA_STAGE;
}


void StandardRequests::handle_in_transfer(Endpoint &ep)
{
}


void StandardRequests::handle_out_transfer(Endpoint &ep)
{
}