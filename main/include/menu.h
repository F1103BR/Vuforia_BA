//Zum Erstellen der gesamten Benutzeroberfläche. Samt einlesen des Inkrementalgebers, und Übergabe von Parametern an "Backend"
#ifndef MENU_H
#define MENU_H



extern const char *OutCombi_names[];

void configure_OLED();
void config_internal_Encoder();
void menu_loop();

#endif