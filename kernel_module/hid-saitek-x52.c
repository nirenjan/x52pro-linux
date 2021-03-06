/*
 * HID driver for Saitek X52 HOTAS
 *
 * Supported devices:
 *  - Saitek X52
 *  - Saitek X52 Pro
 *
 * Copyright (c) 2020 Nirenjan Krishnan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <linux/hid.h>
#include <linux/module.h>
#include <linux/bits.h>

#define VENDOR_SAITEK 0x06a3
#define DEV_X52_1 0x0255
#define DEV_X52_2 0x075c
#define DEV_X52_PRO 0x0762

static void _parse_axis_report(struct input_dev *input_dev,
                               int is_pro, u8 *data, int len)
{
    static const s32 hat_to_axis[16][2] = {
        {0, 0},
        {0, -1},
        {1, -1},
        {1, 0},
        {1, 1},
        {0, 1},
        {-1, 1},
        {-1, 0},
        {-1, -1},
    };

    u8 hat = (data[len - 2]) >> 4;

    u32 axis = (data[3] << 24) |
               (data[2] << 16) |
               (data[1] <<  8) |
               data[0];

    if (is_pro) {
        input_report_abs(input_dev, ABS_X, (axis & 0x3ff));
        input_report_abs(input_dev, ABS_Y, ((axis >> 10) & 0x3ff));
    } else {
        input_report_abs(input_dev, ABS_X, (axis & 0x7ff));
        input_report_abs(input_dev, ABS_Y, ((axis >> 11) & 0x7ff));
    }

    input_report_abs(input_dev, ABS_RZ, ((axis >> 22) & 0x3ff));
    input_report_abs(input_dev, ABS_Z, data[4]);
    input_report_abs(input_dev, ABS_RX, data[5]);
    input_report_abs(input_dev, ABS_RY, data[6]);
    input_report_abs(input_dev, ABS_MISC, data[7]);

    /* Mouse stick is always the last byte of the report */
    input_report_abs(input_dev, ABS_TILT_X, data[len-1] & 0xf);
    input_report_abs(input_dev, ABS_TILT_Y, data[len-1] >> 4);

    /* Hat is always the upper nibble of the penultimate byte of the report */
    input_report_abs(input_dev, ABS_HAT0X, hat_to_axis[hat][0]);
    input_report_abs(input_dev, ABS_HAT0Y, hat_to_axis[hat][1]);
}

/**********************************************************************
 * Mapping buttons
 * ===============
 *
 * The X52 and X52 Pro report the buttons in different orders. In order
 * to let a userspace application handle them in a generic fashion, we
 * define a map for buttons to BTN_* defines, so that one button (eg. clutch)
 * will send the same ID on both X52 and X52 Pro
 *
 * The map is defined below.
 **********************************************************************
 */
#define X52_TRIGGER_1       BTN_TRIGGER_HAPPY1
#define X52_TRIGGER_2       BTN_TRIGGER_HAPPY2
#define X52_BTN_FIRE        BTN_TRIGGER_HAPPY3
#define X52_BTN_A           BTN_TRIGGER_HAPPY4
#define X52_BTN_B           BTN_TRIGGER_HAPPY5
#define X52_BTN_C           BTN_TRIGGER_HAPPY6
#define X52_BTN_D           BTN_TRIGGER_HAPPY7
#define X52_BTN_E           BTN_TRIGGER_HAPPY8
#define X52_BTN_PINKIE      BTN_TRIGGER_HAPPY9
#define X52_BTN_T1_UP       BTN_TRIGGER_HAPPY10
#define X52_BTN_T1_DN       BTN_TRIGGER_HAPPY11
#define X52_BTN_T2_UP       BTN_TRIGGER_HAPPY12
#define X52_BTN_T2_DN       BTN_TRIGGER_HAPPY13
#define X52_BTN_T3_UP       BTN_TRIGGER_HAPPY14
#define X52_BTN_T3_DN       BTN_TRIGGER_HAPPY15
#define X52_BTN_CLUTCH      BTN_TRIGGER_HAPPY16
#define X52_MOUSE_LEFT      BTN_TRIGGER_HAPPY17
#define X52_MOUSE_RIGHT     BTN_TRIGGER_HAPPY18
#define X52_MOUSE_FORWARD   BTN_TRIGGER_HAPPY19
#define X52_MOUSE_BACKWARD  BTN_TRIGGER_HAPPY20
#define X52_STICK_POV_N     BTN_TRIGGER_HAPPY21
#define X52_STICK_POV_E     BTN_TRIGGER_HAPPY22
#define X52_STICK_POV_S     BTN_TRIGGER_HAPPY23
#define X52_STICK_POV_W     BTN_TRIGGER_HAPPY24
#define X52_THROT_POV_N     BTN_TRIGGER_HAPPY25
#define X52_THROT_POV_E     BTN_TRIGGER_HAPPY26
#define X52_THROT_POV_S     BTN_TRIGGER_HAPPY27
#define X52_THROT_POV_W     BTN_TRIGGER_HAPPY28
#define X52_MODE_1          BTN_TRIGGER_HAPPY29
#define X52_MODE_2          BTN_TRIGGER_HAPPY30
#define X52_MODE_3          BTN_TRIGGER_HAPPY31
#define X52_BTN_FUNCTION    BTN_TRIGGER_HAPPY32
#define X52_BTN_START_STOP  BTN_TRIGGER_HAPPY33
#define X52_BTN_RESET       BTN_TRIGGER_HAPPY34
#define X52_BTN_PG_UP       BTN_TRIGGER_HAPPY35
#define X52_BTN_PG_DN       BTN_TRIGGER_HAPPY36
#define X52_BTN_UP          BTN_TRIGGER_HAPPY37
#define X52_BTN_DN          BTN_TRIGGER_HAPPY38
#define X52_BTN_MFD_SELECT  BTN_TRIGGER_HAPPY39

static void _parse_button_report(struct input_dev *input_dev,
                                 int is_pro, u8 *data, int num_buttons)
{
    int i;
    int idx;
    int btn;

    // Map X52 buttons from report to fixed button ID
    // This should be defined in the order provided in the report.
    static const int x52_buttons[] = {
        X52_TRIGGER_1,
        X52_BTN_FIRE,
        X52_BTN_A,
        X52_BTN_B,
        X52_BTN_C,
        X52_BTN_PINKIE,
        X52_BTN_D,
        X52_BTN_E,
        X52_BTN_T1_UP,
        X52_BTN_T1_DN,
        X52_BTN_T2_UP,
        X52_BTN_T2_DN,
        X52_BTN_T3_UP,
        X52_BTN_T3_DN,
        X52_TRIGGER_2,
        X52_STICK_POV_N,
        X52_STICK_POV_E,
        X52_STICK_POV_S,
        X52_STICK_POV_W,
        X52_THROT_POV_N,
        X52_THROT_POV_E,
        X52_THROT_POV_S,
        X52_THROT_POV_W,
        X52_MODE_1,
        X52_MODE_2,
        X52_MODE_3,
        X52_BTN_FUNCTION,
        X52_BTN_START_STOP,
        X52_BTN_RESET,
        X52_BTN_CLUTCH,
        X52_MOUSE_LEFT,
        X52_MOUSE_RIGHT,
        X52_MOUSE_FORWARD,
        X52_MOUSE_BACKWARD,
    };

    static const int pro_buttons[] = {
        X52_TRIGGER_1,
        X52_BTN_FIRE,
        X52_BTN_A,
        X52_BTN_B,
        X52_BTN_C,
        X52_BTN_PINKIE,
        X52_BTN_D,
        X52_BTN_E,
        X52_BTN_T1_UP,
        X52_BTN_T1_DN,
        X52_BTN_T2_UP,
        X52_BTN_T2_DN,
        X52_BTN_T3_UP,
        X52_BTN_T3_DN,
        X52_TRIGGER_2,
        X52_MOUSE_LEFT,
        X52_MOUSE_FORWARD,
        X52_MOUSE_BACKWARD,
        X52_MOUSE_RIGHT,
        X52_STICK_POV_N,
        X52_STICK_POV_E,
        X52_STICK_POV_S,
        X52_STICK_POV_W,
        X52_THROT_POV_N,
        X52_THROT_POV_E,
        X52_THROT_POV_S,
        X52_THROT_POV_W,
        X52_MODE_1,
        X52_MODE_2,
        X52_MODE_3,
        X52_BTN_CLUTCH,
        X52_BTN_FUNCTION,
        X52_BTN_START_STOP,
        X52_BTN_RESET,
        X52_BTN_PG_UP,
        X52_BTN_PG_DN,
        X52_BTN_UP,
        X52_BTN_DN,
        X52_BTN_MFD_SELECT,
    };

    const int *btn_map = is_pro ? pro_buttons : x52_buttons ;

    for (i = 0; i < num_buttons; i++) {
        idx = 8 + (i / BITS_PER_BYTE);
        btn = !!(data[idx] & (1 << (i % BITS_PER_BYTE)));
        input_report_key(input_dev, btn_map[i], btn);
    }
}

static int _parse_x52_report(struct input_dev *input_dev,
                             u8 *data, int len)
{
    if (len != 14) {
        return -1;
    }

    _parse_axis_report(input_dev, 0, data, len);
    _parse_button_report(input_dev, 0, data, 34);
    return 0;
}

static int _parse_x52pro_report(struct input_dev *input_dev,
                             u8 *data, int len)
{
    if (len != 15) {
        return -1;
    }

    _parse_axis_report(input_dev, 1, data, len);
    _parse_button_report(input_dev, 1, data, 39);
    return 0;
}

static int x52_raw_event(struct hid_device *dev,
                         struct hid_report *report, u8 *data, int len)
{
    struct input_dev *input_dev = hid_get_drvdata(dev);
    int is_pro = (dev->product == DEV_X52_PRO);
    int ret;

    if (is_pro) {
        ret = _parse_x52pro_report(input_dev, data, len);
    } else {
        ret = _parse_x52_report(input_dev, data, len);
    }
    input_sync(input_dev);
    return ret;
}

static int x52_input_configured(struct hid_device *dev,
                                struct hid_input *input)
{
    struct input_dev *input_dev = input->input;
    int i;
    int max_btn;
    int is_pro = (dev->product == DEV_X52_PRO);
    int max_stick;

    hid_set_drvdata(dev, input_dev);

    set_bit(EV_KEY, input_dev->evbit);
    set_bit(EV_ABS, input_dev->evbit);

    /*
     * X52 has only 34 buttons, X52 Pro has 39. The first 34 buttons are common
     * although the button order differs between the two.
     */
    max_btn = is_pro ? 39 : 34 ;

    for (i = 0; i < max_btn; i++) {
        set_bit(BTN_TRIGGER_HAPPY1 + i, input_dev->keybit);
    }

    /* Both X52 and X52 Pro have the same number of axes, only the ranges vary */

    set_bit(ABS_X, input_dev->absbit);
    set_bit(ABS_Y, input_dev->absbit);
    set_bit(ABS_Z, input_dev->absbit);
    set_bit(ABS_RX, input_dev->absbit);
    set_bit(ABS_RY, input_dev->absbit);
    set_bit(ABS_RZ, input_dev->absbit);
    set_bit(ABS_RZ, input_dev->absbit);
    set_bit(ABS_HAT0X, input_dev->absbit);
    set_bit(ABS_HAT0Y, input_dev->absbit);
    set_bit(ABS_TILT_X, input_dev->absbit);
    set_bit(ABS_TILT_Y, input_dev->absbit);
    set_bit(ABS_MISC, input_dev->absbit);

    max_stick = is_pro ? 1023 : 2047;
    input_set_abs_params(input_dev, ABS_X, 0, max_stick, max_stick >> 8, max_stick >> 4);
    input_set_abs_params(input_dev, ABS_Y, 0, max_stick, max_stick >> 8, max_stick >> 4);
    input_set_abs_params(input_dev, ABS_RZ, 0, 1023, 3, 63);
    input_set_abs_params(input_dev, ABS_RX, 0, 255, 0, 15);
    input_set_abs_params(input_dev, ABS_RY, 0, 255, 0, 15);
    input_set_abs_params(input_dev, ABS_Z, 0, 255, 0, 15);
    input_set_abs_params(input_dev, ABS_MISC, 0, 255, 0, 15);
    input_set_abs_params(input_dev, ABS_HAT0X, -1, 1, 0, 0);
    input_set_abs_params(input_dev, ABS_HAT0Y, -1, 1, 0, 0);
    input_set_abs_params(input_dev, ABS_TILT_X, 0, 15, 0, 0);
    input_set_abs_params(input_dev, ABS_TILT_Y, 0, 15, 0, 0);

    return 0;
}

static int x52_input_mapping(struct hid_device *dev,
                             struct hid_input *input,
                             struct hid_field *field,
                             struct hid_usage *usage,
                             unsigned long **bit,
                             int *max)
{
    /*
     * We are reporting the events in x52_raw_event.
     * Skip the hid-input processing.
     */
    return -1;
}

static const struct hid_device_id x52_devices[] = {
    { HID_USB_DEVICE(VENDOR_SAITEK, DEV_X52_1) },
    { HID_USB_DEVICE(VENDOR_SAITEK, DEV_X52_2) },
    { HID_USB_DEVICE(VENDOR_SAITEK, DEV_X52_PRO) },
    {}
};

MODULE_DEVICE_TABLE(hid, x52_devices);

static struct hid_driver x52_driver = {
    .name = "saitek-x52",
    .id_table = x52_devices,
    .input_mapping = x52_input_mapping,
    .input_configured = x52_input_configured,
    .raw_event = x52_raw_event,
};

module_hid_driver(x52_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Nirenjan Krishnan");
MODULE_DESCRIPTION("HID driver for Saitek X52 HOTAS devices");
