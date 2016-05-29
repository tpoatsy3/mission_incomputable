

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

TextLayer *text_layer;
TextLayer *text_layer2;
TextLayer *text_layer3;
TextLayer *text_layer4;

SimpleMenuLayer *menu_layer;
SimpleMenuLayer *menu_layer2;
SimpleMenuLayer *menu_layer3;

char *str;

int mode = 0;

static SimpleMenuItem section_items[3];
static SimpleMenuItem section_items2[36];
static SimpleMenuItem section_items3[2];

static SimpleMenuSection only_section[1];
static SimpleMenuSection only_section2[1];
static SimpleMenuSection only_section3[1];

void createString(int i);

//next is confirmation -- title textlayer and menusection with yes and no
//then is just textlayer saying success or invalid hex or failure
void confirmString();

void init();

void broadcastReciever(){
    
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

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
            if(a==16)
                str[i]='G';
            if(a==17)
                str[i]='H';
            if(a==18)
                str[i]='I';
            if(a==19)
                str[i]='J';
            if(a==20)
                str[i]='K';
            if(a==21)
                str[i]='L';
            if(a==22)
                str[i]='M';
            if(a==23)
                str[i]='N';
            if(a==24)
                str[i]='O';
            if(a==25)
                str[i]='P';
            if(a==26)
                str[i]='Q';
            if(a==27)
                str[i]='R';
            if(a==28)
                str[i]='S';
            if(a==29)
                str[i]='T';
            if(a==30)
                str[i]='U';
            if(a==31)
                str[i]='V';
            if(a==32)
                str[i]='W';
            if(a==33)
                str[i]='X';
            if(a==34)
                str[i]='Y';
            if(a==35)
                str[i]='Z';
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

static void send_hex() {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    
    if (!iter) {
        // Error creating outbound message
        return;
    }
    
    int value = 1;
    //TODO: send hex value instead
    
    //key should be zero or one based on if it was code drop or player captured
    dict_write_int(iter, 1, &value, sizeof(int), true);
    dict_write_end(iter);
    
    app_message_outbox_send();
}

void confirmPage(){
    //see if is real hex
    //see if successfully sent message to server
    //make string based on that
    send_hex();
    
    char *conf2 = calloc(200, sizeof(char));

    if(mode == 0){
        conf2 = "Confirmed neutralized code ";
    }
    else{
        conf2 = "Captured player with code ";
    }
    char *code = calloc(200, sizeof(char));
    strcpy(code,str);
    strcat(conf2,code);
    window4 = window_create();
    text_layer4 = text_layer_create(GRect(0, 0, 144, 50));
    text_layer_set_text(text_layer4, conf2);
    layer_add_child(window_get_root_layer(window4),
                    text_layer_get_layer(text_layer4));
    window_stack_push(window4, true);
    //also call method to talk to server
    
}
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    text_layer_set_text(text_layer, "Select");
    int a = simple_menu_layer_get_selected_index(menu_layer);
    window_stack_pop(false);
    if(a == 0){
        mode = 0;
        int i = 0;
        if(str[3]==' ')
            createString(i);
        else
            confirmString();
    }
    if(a == 1){
        mode = 1;
        int i = 0;
        if(str[3]==' ')
            createString(i);
        else
            confirmString();
    }
}

static void confirmation_click_handler(ClickRecognizerRef recognizer, void *context) {
    int a = simple_menu_layer_get_selected_index(menu_layer3);
    window_stack_pop(true);
    init();
    if (a==0){
        //push this to the stack first
        confirmPage();
    }
}


static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    menu_layer_set_selected_next((MenuLayer *)menu_layer, true,  MenuRowAlignCenter, true);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    menu_layer_set_selected_next((MenuLayer *)menu_layer, false,  MenuRowAlignCenter, true);
}
static void up_click_handler2(ClickRecognizerRef recognizer, void *context) {
    menu_layer_set_selected_next((MenuLayer *)menu_layer2, true,  MenuRowAlignCenter, true);
}

static void down_click_handler2(ClickRecognizerRef recognizer, void *context) {
    menu_layer_set_selected_next((MenuLayer *)menu_layer2, false,  MenuRowAlignCenter, true);
}

static void up_click_handler3(ClickRecognizerRef recognizer, void *context) {
    menu_layer_set_selected_next((MenuLayer *)menu_layer3, true,  MenuRowAlignCenter, true);
}

static void down_click_handler3(ClickRecognizerRef recognizer, void *context) {
    menu_layer_set_selected_next((MenuLayer *)menu_layer3, false,  MenuRowAlignCenter, true);
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
    
}

static void click_config_provider2(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, letter_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler2);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler2);
    
}

static void click_config_provider3(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, confirmation_click_handler);
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler3);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler3);
    
}

void confirmString(){
    //problem with this because it doesn't clear the string
    char *conf = calloc(200, sizeof(char));
    
    if(mode == 0){
        conf = "Are you sure you wish to neutralize code ";
    }
    else{
        conf = "Are you sure you wish to capture player with code ";
    }
    char *code2 = calloc(200, sizeof(char));
    strcpy(code,str);
    strcat(conf,code2);
    strcat(conf,"?");
    
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
    window_stack_push(window3, true);

}

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
        section_items2[16] = (SimpleMenuItem){
            .title = "G"
        };
        section_items2[17] = (SimpleMenuItem){
            .title = "H"
        };
        section_items2[18] = (SimpleMenuItem){
            .title = "I"
        };
        section_items2[19] = (SimpleMenuItem){
            .title = "J"
        };
        section_items2[20] = (SimpleMenuItem){
            .title = "K"
        };
        section_items2[21] = (SimpleMenuItem){
            .title = "L"
        };
        section_items2[22] = (SimpleMenuItem){
            .title = "M"
        };
        section_items2[23] = (SimpleMenuItem){
            .title = "N"
        };
        section_items2[24] = (SimpleMenuItem){
            .title = "O"
        };
        section_items2[25] = (SimpleMenuItem){
            .title = "P"
        };
        section_items2[26] = (SimpleMenuItem){
            .title = "Q"
        };
        section_items2[27] = (SimpleMenuItem){
            .title = "R"
        };
        section_items2[28] = (SimpleMenuItem){
            .title = "S"
        };
        section_items2[29] = (SimpleMenuItem){
            .title = "T"
        };
        section_items2[30] = (SimpleMenuItem){
            .title = "U"
        };
        section_items2[31] = (SimpleMenuItem){
            .title = "V"
        };
        section_items2[32] = (SimpleMenuItem){
            .title = "W"
        };
        section_items2[33] = (SimpleMenuItem){
            .title = "X"
        };
        section_items2[34] = (SimpleMenuItem){
            .title = "Y"
        };
        section_items2[35] = (SimpleMenuItem){
            .title = "Z"
        };
        only_section2[0] = (SimpleMenuSection)
        {
            .items = section_items2,
            .num_items = 36
        };
        menu_layer2 = simple_menu_layer_create(GRect(0,30,144,150), window2, only_section2, 1, NULL);
        
        menu_layer_set_click_config_onto_window((MenuLayer *) menu_layer2,  window2);
        window_set_click_config_provider(window2, click_config_provider2);
        
        layer_add_child(window_get_root_layer(window2),
                        simple_menu_layer_get_layer(menu_layer2));
        layer_add_child(window_get_root_layer(window2),
                        text_layer_get_layer(text_layer2));
        window_stack_push(window2, false);
    
    //base number of times this is run on the string
}


void init() {

    
    window = window_create();
    //window_set_click_config_provider(window, click_config_provider);
    
    text_layer = text_layer_create(GRect(0, 0, 144, 30));
    section_items[0] = (SimpleMenuItem){
        .title = "Code Drop"
    };
    section_items[1] = (SimpleMenuItem){
        .title = "Capture Player"
    };
    section_items[2] = (SimpleMenuItem){
        .title = "View Messages"
    };
    only_section[0] = (SimpleMenuSection)
    {
        .items = section_items,
        .num_items = 3
    };
    menu_layer = simple_menu_layer_create(GRect(0,30,144,150), window, only_section, 1, NULL);
    
    menu_layer_set_click_config_onto_window((MenuLayer *) menu_layer,  window);
    window_set_click_config_provider(window, click_config_provider);
    
    char *status = "Status";
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
    text_layer_set_text(text_layer, status);
    layer_add_child(window_get_root_layer(window),
                    text_layer_get_layer(text_layer));
    layer_add_child(window_get_root_layer(window),
                    simple_menu_layer_get_layer(menu_layer));
    window_stack_push(window, true);
}

void deinit() {
    text_layer_destroy(text_layer);
    simple_menu_layer_destroy(menu_layer);
    window_destroy(window);
    window_destroy(window2);
}

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

