/*
 * FireBase.h
 *
 *  Created on: 21 thg 4, 2022
 *      Author: A315-56
 */

#ifndef _FIREBASE_H_
#define _FIREBASE_H_


#include "stdio.h"
#include "stdbool.h"
#include "esp_http_client.h"



void FireBase_Init(char *URL);

void FireBase_SetInt(char *Path, int DataInt);
void FireBase_SetDouble(char *Path, double DataFloat);
void FireBase_SetBool(char *Path, bool DataBool);
void FireBase_SetString(char *Path, char *DataString);

int    FireBase_GetInt(char *Path);
double FireBase_GetDouble(char *Path);
bool   FireBase_GetBool(char *Path);
char  *FireBase_GetString(char *Path);


#endif /* _FIREBASE_H_ */
