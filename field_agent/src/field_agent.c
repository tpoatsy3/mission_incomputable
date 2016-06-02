

#include <pebble.h>
#include <pebble_fonts.h>
#include <stdbool.h>
#include "mission.h"

//primary window
Window *window;

//text entry window
Window *window2;

//confirmation window
Window *window3;

//confirmation page window
Window *window4;

//status page window
Window *window5;

//message page window
Window *window6;

TextLayer *text_layer;
TextLayer *text_layer2;
TextLayer *text_layer3;
TextLayer *text_layer4;
//compass textlayer
TextLayer *text_layer5;
//messages header textlayer
TextLayer *text_layer6;
//status textlayer
TextLayer *text_layer7;
//health
TextLayer *text_layer8;

uint32_t num_samples = 3;

static HealthMetric metric = HealthMetricStepCount;
static HealthMetric metric2 = HealthMetricActiveKCalories;

int startsteps;
int startcals;

//compass
TextLayer *text_layer5;

SimpleMenuLayer *menu_layer;
SimpleMenuLayer *menu_layer2;
SimpleMenuLayer *menu_layer3;
SimpleMenuLayer *menu_layer4;

char *latitude = "";
char *longitude = "";
char *pebbleID;

int steps;
int calories;

int nummessages = 0; //number of messages
//for status updates

//health metric

AccelData compareAccel;

static GBitmap *code_bitmap = NULL;
static GBitmap *capture_bitmap = NULL;
static GBitmap *code_big_bitmap = NULL;
static GBitmap *capture_big_bitmap = NULL;
static GBitmap *messages_bitmap = NULL;
static GBitmap *status_bitmap = NULL;
static GBitmap *check_bitmap = NULL;


static uint32_t MENU_ICONS[] = {
    RESOURCE_ID_IMAGE_CODE,
    RESOURCE_ID_IMAGE_CAPTURE,
    RESOURCE_ID_IMAGE_MESSAGES,
    RESOURCE_ID_IMAGE_STATUS,
    RESOURCE_ID_IMAGE_CODE_BIG,
    RESOURCE_ID_IMAGE_CAPTURE_BIG,
    RESOURCE_ID_IMAGE_CHECK
};


char *str;
char *status;
//char *conf2;

int direction = 0;

int mode = 0;

static SimpleMenuItem section_items[4];
static SimpleMenuItem section_items2[16];
static SimpleMenuItem section_items3[2];

static SimpleMenuSection only_section[1];
static SimpleMenuSection only_section2[1];
static SimpleMenuSection only_section3[1];

void createString(int i);

//next is confirmation -- title textlayer and menusection with yes and no
//then is just textlayer saying success or invalid hex or failure
void confirmString();
void init2();
void send(char *opcode, int statusReq);

void init();

static HealthMetric metric;
int health_get_metric_sum(HealthMetric metric);

void broadcastReciever(){
    
}
/**************** functions ****************/

/******** messagePage() ************/

/*
 * This function creates the page to view the most recent messages recieved by the server
 */
void messagePage(){
    window6 = window_create();
    //window_set_click_config_provider(window, click_config_provider);
    static SimpleMenuItem section_items4[10];
    //create hashtable
    //call by size
    section_items4[0] = (SimpleMenuItem){
            .title = "Neturalized 34FA"
    };
    section_items4[1] = (SimpleMenuItem){
        .title = "Neutralized 4B2D"
    };
    section_items4[2] = (SimpleMenuItem){
        .title = "Captured 4B2D"
    };
    section_items4[3] = (SimpleMenuItem){
        .title = "Captured 4B2D"
    };
    section_items4[4] = (SimpleMenuItem){
        .title = "Neutralized 4B2D"
    };
    section_items4[5] = (SimpleMenuItem){
        .title = "Captured 4B2D"
    };
    section_items4[6] = (SimpleMenuItem){
        .title = "Captured 4B2D"
    };
    section_items4[7] = (SimpleMenuItem){
        .title = "Captured 4A5D"
    };
    section_items4[8] = (SimpleMenuItem){
        .title = "Captured 4A5D"
    };
    section_items4[9] = (SimpleMenuItem){
        .title = "Captured 4A5D"
    };
    text_layer6 = text_layer_create(GRect(0, 0, 144, 30));

    only_section3[0] = (SimpleMenuSection)
    {
        .items = section_items4,
        .num_items = 10
    };
    menu_layer4 = simple_menu_layer_create(GRect(0,30,144,130), window6, only_section3, 1, NULL);
    
    text_layer_set_font(text_layer6, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
    text_layer_set_text_alignment(text_layer6, GTextAlignmentCenter);
    text_layer_set_background_color(text_layer6,GColorDarkCandyAppleRed);
    text_layer_set_text_color(text_layer6, GColorWhite);
    text_layer_set_text(text_layer6,"Messages");
    layer_add_child(window_get_root_layer(window6),
                    text_layer_get_layer(text_layer6));
    layer_add_child(window_get_root_layer(window6),
                    simple_menu_layer_get_layer(menu_layer4));
    window_stack_push(window6, true);
}

/******** statusPage() ************/

/*
 * This function creates a window to view the status of the field agent and does this by
 * requesting the game status.
 */
void statusPage(){
    //see if is real hex
    //see if successfully sent message to server
    //make string based on that
    
    
    //send message to server to recieve message
    char *stat = "Status: Active";
    BitmapLayer *image2 = bitmap_layer_create(GRect(0, 30, 144, 100));
    //graphics_draw_bitmap_in_rect(void, code_bitmap, GRect rect)
    bitmap_layer_set_bitmap(image2, check_bitmap);
    //conf2 = "confirmed";
    LOG("MADE");
    send("",1);
    
    
    
    window5 = window_create();
    text_layer7 = text_layer_create(GRect(0, 0, 144, 30));
    text_layer_set_text(text_layer7, stat);
    text_layer_set_font(text_layer7, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
    text_layer_set_text_alignment(text_layer7, GTextAlignmentCenter);
    text_layer_set_background_color(text_layer7,GColorDarkCandyAppleRed);
    text_layer_set_text_color(text_layer7, GColorWhite);
    
    layer_add_child(window_get_root_layer(window5),
                    text_layer_get_layer(text_layer7));
    layer_add_child(window_get_root_layer(window5),
                    bitmap_layer_get_layer(image2));
    window_stack_push(window5, true);
    //also call method to talk to server
    
}

/******** letter_click_handler() ************/

/*
 * This function creates a view to choose a hex digit from 0-9 and A-F. Each chosen
 * character is stored in a string to forward to the server, and this window is called
 * four times for each character in the four-digit hex code.
 */

static void letter_click_handler(ClickRecognizerRef recognizer, void *context) {
    int a = simple_menu_layer_get_selected_index(menu_layer2);
    
    for(int i = 0; i<4; i++){
        if(str[i]==' '){
            if(a==0)
                str[i]='0';
            if(a==1)
                str[i]='1';
            if(a==2)
                str[i]='2';
            if(a==3)
                str[i]='3';
            if(a==4)
                str[i]='4';
            if(a==5)
                str[i]='5';
            if(a==6)
                str[i]='6';
            if(a==7)
                str[i]='7';
            if(a==8)
                str[i]='8';
            if(a==9)
                str[i]='9';
            if(a==10)
                str[i]='A';
            if(a==11)
                str[i]='B';
            if(a==12)
                str[i]='C';
            if(a==13)
                str[i]='D';
            if(a==14)
                str[i]='E';
            if(a==15)
                str[i]='F';
            if(str[i+1]==' '){
                window_stack_pop(false);
                createString(i);
            }
            else{
                window_stack_pop(false);
                confirmString();
            }
            //text_layer_set_text(text_layer2, str);
            break;
        }
    }
}

/******** formatMessage() ************/

/*
 * This function formats the message in its proper way.
 */

void formatMessage(char *message, int statusReq){
    //FA_LOCATION|gameId|pebbleId|teamName|playerName|lat|long|statusReq
    snprintf(message, sizeof(char)*200,"FA_LOCATION|%s|%s|%s|%s|%s|%s|%s"," "," "," "," "," "," "," ");
    
    //put in 7 strings

}

/******** send() ************/

/*
 * This function sends a message with the proper opcode to the server, determining whether a status request is
 * neccessary as well.
 */
void send(char *opcode, int statusReq) {

    
    // Declare the dictionary's iterator
    DictionaryIterator *iter;
    char *message = calloc(8000,sizeof(char));
    
    //FA_LOCATION|gameId|pebbleId|teamName|playerName|lat|long|statusReq
    snprintf(message, sizeof(char)*200,"%s|%s|%s|%s|%s|%s|%s|%d",opcode," "," "," "," "," "," ",statusReq);
    

    // Prepare the outbox buffer for this message
    AppMessageResult result = app_message_outbox_begin(&iter);
    
    if(result == APP_MSG_OK) {
        //conf2 = "app message okay";
        // Construct & send the message.
        dict_write_int(iter, 1, message, sizeof(int), true);
        result = app_message_outbox_send();
        if(result != APP_MSG_OK) {
            //conf2 = "error1";
            // handle error!
        }
    } else {
        // handle error!
        //conf2 = "error2";
    }
    free(message);
}

/******** send() ************/

/*
 * This function sends a message to the server with key being the proper enum value and data being the correct string.
 */
void sendMessage(int key, char *data){
    DictionaryIterator *iter;
    
    // Prepare the outbox buffer for this message
    AppMessageResult result = app_message_outbox_begin(&iter);
    
    if(result == APP_MSG_OK) {
        //conf2 = "app message okay";
        // Construct & send the message.
        dict_write_int(iter, key, data, sizeof(int), true);
        result = app_message_outbox_send();
        if(result != APP_MSG_OK) {
            //conf2 = "error1";
            // handle error!
        }
    } else {

    }
    
}

/******** send() ************/

/*
 * This function creates a page to confirm whether the message was successfully sent to the server.
 */
void confirmPage(){
    //see if is real hex
    //see if successfully sent message to server
    //make string based on that

    char *conf2 = calloc(200, sizeof(char));
    conf2 = "";
    BitmapLayer *image = bitmap_layer_create(GRect(0, 30, 144, 100));
    if(mode == 0){
        //conf2 = "Confirmed neutralized code ";
        //snprintf(conf2, sizeof(char)*33, "Confirmed neutralized code %s!",str);
        conf2 = "Neutralized code!";
        //graphics_draw_bitmap_in_rect(void, code_bitmap, GRect rect)
        bitmap_layer_set_bitmap(image, code_big_bitmap);
    }
    else{
        //snprintf(conf2, sizeof(char)*34, "Captured player with code %s!",str);
        conf2 = "Captured player!";
        bitmap_layer_set_bitmap(image, capture_big_bitmap);
    }
    

    //conf2 = "confirmed";
    LOG("MADE");
    send("FA_LOCATION",1);
    text_layer_set_text(text_layer, "Active");

    window4 = window_create();
    text_layer4 = text_layer_create(GRect(0, 0, 144, 50));
    text_layer_set_text(text_layer4, conf2);
    
    text_layer_set_font(text_layer4, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
    text_layer_set_text_alignment(text_layer4, GTextAlignmentCenter);
    
    layer_add_child(window_get_root_layer(window4),
                    text_layer_get_layer(text_layer4));
    layer_add_child(window_get_root_layer(window4),
                    bitmap_layer_get_layer(image));
    for(int i = 0; i <4;i++){
        str[i]=' ';
    }
    window_stack_push(window4, true);
    //also call method to talk to server
    
}

/******** select_click_handler() ************/

/*
 * The select button handler for the home page menu.
 */
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    //text_layer_set_text(text_layer, "Select");
    int a = simple_menu_layer_get_selected_index(menu_layer);
    //window_stack_pop(false);
    //code drop
    if(a == 0){
        mode = 0;
        int i = 0;
        if(str[3]==' ')
            createString(i);
        else
            confirmString();
    }
    //capture player
    if(a == 1){
        mode = 1;
        int i = 0;
        if(str[3]==' ')
            createString(i);
        else
            confirmString();
    }
    //request status
    if(a==2){
        statusPage();
    }
    //messages
    if(a==3){
        messagePage();
    }
}

/******** confirmation_click_handler() ************/

/*
 * The select button handler for the page asking whether or not you wish to go
 * through with an action.
 */
static void confirmation_click_handler(ClickRecognizerRef recognizer, void *context) {
    int a = simple_menu_layer_get_selected_index(menu_layer3);
    window_stack_pop(true);
    //init();
    if (a==0){
        //push this to the stack first
        confirmPage();
    }
}

/******** up_click_handler() ************/

/*
 * An up click handler to reproduce proper menu behavior because all clicks are transferred
 * to the window. This up button moves the highlighted menu item to the one above.
 */
static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    menu_layer_set_selected_next((MenuLayer *)menu_layer, true,  MenuRowAlignCenter, true);
}

/******** down_click_handler() ************/

/*
 * A down click handler to reproduce proper menu behavior because all clicks are transferred
 * to the window. This down button moves the highlighted menu item to the one below.
 */
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    menu_layer_set_selected_next((MenuLayer *)menu_layer, false,  MenuRowAlignCenter, true);
}

/******** up_click_handler2() ************/

/*
 * An up click handler to reproduce proper menu behavior because all clicks are transferred
 * to the window. This up button moves the highlighted menu item to the one above.
 */
static void up_click_handler2(ClickRecognizerRef recognizer, void *context) {
    menu_layer_set_selected_next((MenuLayer *)menu_layer2, true,  MenuRowAlignCenter, true);
}

/******** down_click_handler2() ************/

/*
 * A down click handler to reproduce proper menu behavior because all clicks are transferred
 * to the window. This down button moves the highlighted menu item to the one below.
 */
static void down_click_handler2(ClickRecognizerRef recognizer, void *context) {
    menu_layer_set_selected_next((MenuLayer *)menu_layer2, false,  MenuRowAlignCenter, true);
}

/******** up_click_handler3() ************/

/*
 * An up click handler to reproduce proper menu behavior because all clicks are transferred
 * to the window. This up button moves the highlighted menu item to the one above.
 */
static void up_click_handler3(ClickRecognizerRef recognizer, void *context) {
    menu_layer_set_selected_next((MenuLayer *)menu_layer3, true,  MenuRowAlignCenter, true);
}

/******** down_click_handler3() ************/

/*
 * A down click handler to reproduce proper menu behavior because all clicks are transferred
 * to the window. This down button moves the highlighted menu item to the one below.
 */
static void down_click_handler3(ClickRecognizerRef recognizer, void *context) {
    menu_layer_set_selected_next((MenuLayer *)menu_layer3, false,  MenuRowAlignCenter, true);
}

/******** click_config_provider() ************/

/*
 * Click configuration settings for the home screen menu.
 */
static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    
}

/******** click_config_provider2() ************/

/*
 * Click configuration settings for the keyboard menu
 */
static void click_config_provider2(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, letter_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler2);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler2);
    
}

/******** click_config_provider3() ************/

/*
 * Click configuration settings confirmation window.
 */
static void click_config_provider3(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, confirmation_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler3);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler3);
    
}

/******** confirmString() ************/

/*
 * A class that creates a window asking if the user wishes to proceed with the action.
 */
void confirmString(){
    char *conf = calloc(200, sizeof(char));
    if(mode == 0){
        snprintf(conf, sizeof(char)*49, "Are you sure you wish to neutralize code %s ?",str);
    }
    else{
        snprintf(conf, sizeof(char)*58, "Are you sure you wish to capture player with code %s ?",str);
    }
    
    window3 = window_create();
    text_layer3 = text_layer_create(GRect(0, 0, 144, 50));
    text_layer_set_text(text_layer3, conf);
    section_items3[0] = (SimpleMenuItem){
        .title = "Yes"
    };
    section_items3[1] = (SimpleMenuItem){
        .title = "No"
    };
    only_section3[0] = (SimpleMenuSection)
    {
        .items = section_items3,
        .num_items = 2
    };
    menu_layer3 = simple_menu_layer_create(GRect(0,50,144,130), window3, only_section3, 1, NULL);
    menu_layer_set_click_config_onto_window((MenuLayer *) menu_layer3,  window3);
    window_set_click_config_provider(window3, click_config_provider3);
    
    layer_add_child(window_get_root_layer(window3),
                    simple_menu_layer_get_layer(menu_layer3));
    layer_add_child(window_get_root_layer(window3),
                    text_layer_get_layer(text_layer3));
    text_layer_set_text(text_layer, "Active");
    window_stack_push(window,false);
    window_stack_push(window3, true);

}

/******** createString() ************/

/*
 * A class that creates a string from items selected in a drop
 * down menu. It is called four times to make a four-character string.
 */
void createString(int i){
        window2 = window_create();
        text_layer2 = text_layer_create(GRect(0, 0, 144, 30));
        text_layer_set_text(text_layer2, str);
        
        section_items2[0] = (SimpleMenuItem){
            .title = "0"
        };
        section_items2[1] = (SimpleMenuItem){
            .title = "1"
        };
        section_items2[2] = (SimpleMenuItem){
            .title = "2"
        };
        section_items2[3] = (SimpleMenuItem){
            .title = "3"
        };
        section_items2[4] = (SimpleMenuItem){
            .title = "4"
        };
        section_items2[5] = (SimpleMenuItem){
            .title = "5"
        };
        section_items2[6] = (SimpleMenuItem){
            .title = "6"
        };
        section_items2[7] = (SimpleMenuItem){
            .title = "7"
        };
        section_items2[8] = (SimpleMenuItem){
            .title = "8"
        };
        section_items2[9] = (SimpleMenuItem){
            .title = "9"
        };
        section_items2[10] = (SimpleMenuItem){
            .title = "A"
        };
        section_items2[11] = (SimpleMenuItem){
            .title = "B"
        };
        section_items2[12] = (SimpleMenuItem){
            .title = "C"
        };
        section_items2[13] = (SimpleMenuItem){
            .title = "D"
        };
        section_items2[14] = (SimpleMenuItem){
            .title = "E"
        };
        section_items2[15] = (SimpleMenuItem){
            .title = "F"
        };

        only_section2[0] = (SimpleMenuSection)
        {
            .items = section_items2,
            .num_items = 16
        };
        menu_layer2 = simple_menu_layer_create(GRect(0,30,144,150), window2, only_section2, 1, NULL);
        
        menu_layer_set_click_config_onto_window((MenuLayer *) menu_layer2,  window2);
        window_set_click_config_provider(window2, click_config_provider2);
        menu_layer_set_highlight_colors((MenuLayer *) menu_layer2, GColorDarkCandyAppleRed, GColorWhite);
        layer_add_child(window_get_root_layer(window2),
                        simple_menu_layer_get_layer(menu_layer2));
        layer_add_child(window_get_root_layer(window2),
                        text_layer_get_layer(text_layer2));
        window_stack_pop(false);
        window_stack_push(window2, false);
    //base number of times this is run on the string
}


/******** tick_handler() ************/

/*
 * A class to keep track of the time that has passed since the app was started to send
 * periodic updates to the server.
 */
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    //update_time();
    if(tick_time->tm_sec % 300 == 0){
        //send calories and steps
    }
    //should send every fifteen seconds if it is on the move
    else if(tick_time->tm_sec % 15 == 0) {
        //strcat(status,"!");
        //also send coordinates
        //also send direction
        
        AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
        accel_service_peek(&accel);
        if(compareAccel.x==-1&&compareAccel.y==-1&&compareAccel.z==-1){
            compareAccel.x = accel.x;
            compareAccel.y = accel.y;
            compareAccel.z = accel.z;
        }
        else{
            //compare the distances
            double diffx = compareAccel.x-accel.x;
            double diffy = compareAccel.y-accel.y;
            if((diffy>.5||diffy<-.5)||(diffx>.5||diffx<-.5)){
                status = "Moving";
            }
        }
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);
        
        // Add a key-value pair
        dict_write_uint8(iter, 0, 0);
        
        // Send the message!
        app_message_outbox_send();
        
        //send compass
        send("FA_DIRECTION",direction);
    }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    //extract whatever was sent
    //extract tuple because it will send key: info
    APP_LOG(APP_LOG_LEVEL_INFO, "Inbox recieved success!");
    //this is where messages are recieved
    
    // Check: Is this a PebbleKit JS ready message?! If so, then it is safe to send messages!
    Tuple *ready_tuple = dict_find(iterator, AppKeyJSReady);
    if(ready_tuple) {
        // Log the value sent as part of the received message.
        char *ready_str = ready_tuple->value->cstring;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Got AppKeyJSReady: %s", ready_str);
    }
    //send should be somewhere else

    //message recieved
    Tuple *recieved_tuple = dict_find(iterator, AppKeyRecvMsg);
    if(recieved_tuple) {
        // Log the value sent as part of the received message.
        char *ready_str = recieved_tuple->value->cstring;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Got AppKeyJSReady: %s", ready_str);
    }
    //location tuple
    Tuple *location_tuple = dict_find(iterator, AppKeyLocation);
    if(location_tuple) {
        // Log the value sent as part of the received message.
        char *ready_str = location_tuple->value->cstring;
        char* token = strtok(ready_str, "|");
        latitude = token;
        longitude = token = strtok(NULL, "|");
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Got AppKeyJSReady: %s", ready_str);
    }
    //pebble id
    Tuple *pebble_tuple = dict_find(iterator, AppKeyPebbleId);
    if(pebble_tuple) {
        // Log the value sent as part of the received message.
        char *ready_str = location_tuple->value->cstring;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Got AppKeyJSReady: %s", ready_str);
    }
    //error sending the key
    Tuple *error_tuple = dict_find(iterator, AppKeySendError);
    if(error_tuple) {
        // Log the value sent as part of the received message.
        char *ready_str = location_tuple->value->cstring;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Got AppKeyJSReady: %s", ready_str);
    }
    
    // ...other "checks"...
}

/******** inbox_dropped_callback() ************/

/*
 * A class called when the watch drops a message sent from the server.
 */
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
    //should send enums at first
}

/******** outbox_failed_callback() ************/

/*
 * A class called when the output fails to send a message to the server.
 */
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

/******** outbox_failed_callback() ************/

/*
 * A class called when the output has sent a message to the server.
 */
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

/******** compass_heading_handler() ************/

/*
 * A class to keep track of the direction that the phone is pointing in.
 */
static void compass_heading_handler(CompassHeadingData heading_data) {
    // display heading in degrees and radians
    static char s_heading_buf[64];
    snprintf(s_heading_buf, sizeof(s_heading_buf),
             "Direction: %ld°\n%ld.%02ldpi",
             TRIGANGLE_TO_DEG(heading_data.magnetic_heading),
             // display radians, units digit
             (TRIGANGLE_TO_DEG(heading_data.magnetic_heading) * 2) / 360,
             // radians, digits after decimal
             ((TRIGANGLE_TO_DEG(heading_data.magnetic_heading) * 200) / 360) % 100
             );
    direction = TRIGANGLE_TO_DEG(heading_data.magnetic_heading);

    text_layer_set_text(text_layer5, s_heading_buf);

}


/******** health_get_metric_sum() ************/

/*
 * A class to help getting the health data from the pebble.
 */
int health_get_metric_sum(HealthMetric metric) {
    HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric,
                                                                           time_start_of_today(), time(NULL));
    if(mask == HealthServiceAccessibilityMaskAvailable) {
        return (int)health_service_sum_today(metric);
    } else {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
        return 0;
    }
}

/******** health_handler() ************/

/*
 * A class to get health data and place it on the home screen of the app.
 */
static void health_handler(HealthEventType event, void *context) {
    
    steps = (health_get_metric_sum(metric)-startsteps);
    calories = (health_get_metric_sum(metric2)-startcals);
    static char healthstring[300];
    snprintf(healthstring, sizeof(healthstring),
             "Steps: %d Cals: %d",steps,calories);
    
    text_layer_set_text(text_layer8, healthstring);
}
/******** home() ************/

/*
 * A class to create the home window of the app
 */
void home(){
    code_bitmap = gbitmap_create_with_resource(MENU_ICONS[0]);
    capture_bitmap = gbitmap_create_with_resource(MENU_ICONS[1]);
    messages_bitmap = gbitmap_create_with_resource(MENU_ICONS[2]);
    status_bitmap = gbitmap_create_with_resource(MENU_ICONS[3]);
    code_big_bitmap = gbitmap_create_with_resource(MENU_ICONS[4]);
    capture_big_bitmap = gbitmap_create_with_resource(MENU_ICONS[5]);
    check_bitmap = gbitmap_create_with_resource(MENU_ICONS[6]);
    
    // Make sure the time is displayed from the start

    window = window_create();
    //window_set_click_config_provider(window, click_config_provider);
    
    text_layer = text_layer_create(GRect(0, 0, 144, 30));
    text_layer5 = text_layer_create(GRect(0, 30, 144, 20));
    text_layer8 = text_layer_create(GRect(0, 50, 144, 20));
    section_items[0] = (SimpleMenuItem){
        .title = "Code Drop",
        .icon = code_bitmap
    };
    section_items[1] = (SimpleMenuItem){
        .title = "Capture Player",
        .icon = capture_bitmap
    };
    section_items[3] = (SimpleMenuItem){
        .title = "View Messages",
        .icon = messages_bitmap
    };
    section_items[2] = (SimpleMenuItem){
        .title = "Get Status",
        .icon = status_bitmap
    };
    only_section[0] = (SimpleMenuSection)
    {
        .items = section_items,
        .num_items = 4
    };
    menu_layer = simple_menu_layer_create(GRect(0,70,144,130), window, only_section, 1, NULL);
    
    menu_layer_set_click_config_onto_window((MenuLayer *) menu_layer,  window);
    window_set_click_config_provider(window, click_config_provider);
    
    status = "Active";
    
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
    text_layer_set_text(text_layer, "Active");
    text_layer_set_background_color(text_layer,GColorDarkCandyAppleRed);
    text_layer_set_text_color(text_layer, GColorWhite);
    
    TextLayer *text_layer10 = text_layer_create(GRect(0, 0, 144, 30));
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
    text_layer_set_text_alignment(text_layer10, GTextAlignmentCenter);
    text_layer_set_text(text_layer10, "Active");
    text_layer_set_background_color(text_layer,GColorDarkCandyAppleRed);
    text_layer_set_text_color(text_layer10, GColorWhite);

    
    //replace with the direction
    text_layer_set_text(text_layer5,"Direction");
    layer_add_child(window_get_root_layer(window),
                    text_layer_get_layer(text_layer));
    layer_add_child(window_get_root_layer(window),
                    text_layer_get_layer(text_layer5));
    layer_add_child(window_get_root_layer(window),
                    text_layer_get_layer(text_layer8));
    layer_add_child(window_get_root_layer(window),
                    simple_menu_layer_get_layer(menu_layer));
    window_stack_push(window, true);

}
/******** init() ************/

/*
 * A class to initialize the app and set up many of the handlers.
 */
void init() {
    health_service_events_subscribe(health_handler, NULL);

    
    startsteps = health_get_metric_sum(metric);
    startcals = health_get_metric_sum(metric2);
    tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

    compareAccel = (AccelData) { .x = -1, .y = -1, .z = -1 };
    
    //compass
    compass_service_set_heading_filter(DEG_TO_TRIGANGLE(2));
    compass_service_subscribe(&compass_heading_handler);
    //compass
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
    home();
}

/******** deinit() ************/

/*
 * A class to deinitialize the app and destroy the windows.
 */
void deinit() {
    text_layer_destroy(text_layer);
    simple_menu_layer_destroy(menu_layer);
    window_destroy(window);
    window_destroy(window2);
    accel_data_service_unsubscribe();
}

/******** main() ************/

/*
 * The main loop of the Pebble watch.
 */
int main() {
    str = malloc(5*sizeof(char));
    memset(str,' ',4);
    str[4]='\0';
    init();
    LOG("field_agent initialized!");
    app_event_loop();
    LOG("field_agent deinitialized!");
    deinit();
    return 0;
}

