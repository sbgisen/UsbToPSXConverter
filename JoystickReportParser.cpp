/* Copyright (C) 2011 Circuits At Home, LTD. All rights reserved.

This software may be distributed and modified under the terms of the GNU
General Public License version 2 (GPL2) as published by the Free Software
Foundation and appearing in the file GPL2.TXT included in the packaging of
this file. Please note that GPL2 Section 2[b] requires that all works based
on this software must also be made publicly available under the terms of
the GPL2 ("Copyleft").

Contact information
-------------------

Circuits At Home, LTD
Web      :  http://www.circuitsathome.com
e-mail   :  support@circuitsathome.com
 */

#include "JoystickReportParser.h"

uint8_t JoystickDescParser::ParseItem(uint8_t **pp, uint16_t *pcntdn) {
        //uint8_t ret = enErrorSuccess;

        switch(itemParseState) {
                case 0:
                        if(**pp == HID_LONG_ITEM_PREFIX)
                                USBTRACE("\r\nLONG\r\n");
                        else {
                                uint8_t size = ((**pp) & DATA_SIZE_MASK);
                                itemPrefix = (**pp);
                                itemSize = 1 + ((size == DATA_SIZE_4) ? 4 : size);
                        }
                        (*pp)++;
                        (*pcntdn)--;
                        itemSize--;
                        itemParseState = 1;

                        if(!itemSize)
                                break;

                        if(!pcntdn)
                                return enErrorIncomplete;
                case 1:
                        theBuffer.valueSize = itemSize;
                        valParser.Initialize(&theBuffer);
                        itemParseState = 2;
                case 2:
                        if(!valParser.Parse(pp, pcntdn))
                                return enErrorIncomplete;
                        itemParseState = 3;
                case 3:
                {
                        uint8_t data = *((uint8_t*)varBuffer);

                        switch(itemPrefix & (TYPE_MASK | TAG_MASK)) {
                                case (TYPE_LOCAL | TAG_LOCAL_USAGE):
                                        if(pfUsage) {
                                                if(theBuffer.valueSize > 1) {
                                                        uint16_t* ui16 = reinterpret_cast<uint16_t *>(varBuffer);
                                                        pfUsage(*ui16);
                                                } else{
                                                        pfUsage(data);
                                                }
                                        }
                                        usage[usageIndex++] = data;
                                        break;
                                case (TYPE_GLOBAL | TAG_GLOBAL_REPORTSIZE):
                                        rptSize = data;
                                        break;
                                case (TYPE_GLOBAL | TAG_GLOBAL_REPORTCOUNT):
                                        rptCount = data;
                                        break;
                                case (TYPE_GLOBAL | TAG_GLOBAL_REPORTID):
                                        rptId = data;
                                        break;
                                case (TYPE_LOCAL | TAG_LOCAL_USAGEMIN):
                                        useMin = data;
                                        break;
                                case (TYPE_LOCAL | TAG_LOCAL_USAGEMAX):
                                        useMax = data;
                                        break;
                                case (TYPE_GLOBAL | TAG_GLOBAL_USAGEPAGE):
                                        SetUsagePage(data);
                                        break;
                                case (TYPE_MAIN | TAG_MAIN_OUTPUT):
                                case (TYPE_MAIN | TAG_MAIN_FEATURE):
                                case (TYPE_MAIN | TAG_MAIN_COLLECTION):
                                case (TYPE_MAIN | TAG_MAIN_ENDCOLLECTION):
                                        // rptSize = 0;
                                        // rptCount = 0;
                                        
                                        ZeroMemory(usageBufferSize, usage);
                                        usageIndex = 0;
                                        useMin = 0;
                                        useMax = 0;
                                        break;
                                case (TYPE_MAIN | TAG_MAIN_INPUT):
                                        OnInputItem(data);

                                        totalSize += (uint16_t)rptSize * (uint16_t)rptCount;

                                        // rptSize = 0;
                                        // rptCount = 0;
                                        ZeroMemory(usageBufferSize, usage);
                                        usageIndex = 0;
                                        useMin = 0;
                                        useMax = 0;
                                        break;
                        } // switch (**pp & (TYPE_MASK | TAG_MASK))
                }
        } // switch (itemParseState)
        itemParseState = 0;
        return enErrorSuccess;
}

void JoystickDescParser::OnInputItem(uint8_t itm) {
        uint8_t byte_offset = (totalSize >> 3); // calculate offset to the next unhandled byte i = (int)(totalCount / 8);
        uint8_t *p = pBuf + byte_offset; // current byte pointer

        uint8_t usageID = useMin;

        bool print_usemin_usemax = ((useMin < useMax) && ((itm & 3) == 2) && pfUsage) ? true : false;

        // for each field in field array defined by rptCount
        for(uint8_t field = 0; field < rptCount; field++, usageID++) {

                union {
                        uint8_t bResult[4];
                        uint16_t wResult[2];
                        uint32_t dwResult;
                } result;

                result.dwResult = 0;
                uint8_t mask = 0;

                if(print_usemin_usemax)
                {
                        E_Notify(PSTR("\r\n"), 0x80);
                        pfUsage(usageID);
                }

                // bits_left            - number of bits in the field(array of fields, depending on Report Count) left to process
                // bits_of_byte         - number of bits in current byte left to process
                // bits_to_copy         - number of bits to copy to result buffer

                // for each bit in a field
                for (uint8_t bits_left = rptSize, bits_to_copy, index = 0; bits_left > 0;
                        bits_left -= bits_to_copy){
                        uint8_t val = *p;
                        val >>= (8 - bits_of_byte); // Shift by the number of bits already processed

                        bits_to_copy = (bits_left > bits_of_byte) ? bits_of_byte : bits_left;

                        mask = 0;
                        for (uint8_t j = bits_to_copy; j; j--)
                        {
                                mask <<= 1;
                                mask |= 1;
                        }

                        result.bResult[index] = (result.bResult[index] | (val & mask));

                        bits_of_byte -= bits_to_copy;

                        if (bits_of_byte < 1)
                        {
                                bits_of_byte = 8;
                                p++;
                                index++;
                        }
                }

                switch(usage[field]? usage[field]: usageID){
                case 0x30: eventData.axis.X = result.dwResult; break;
                case 0x31: eventData.axis.Y = result.dwResult; break;
                case 0x32: eventData.axis.Z = result.dwResult; break;
                case 0x33: eventData.axis.Rx = result.dwResult; break;
                case 0x34: eventData.axis.Ry = result.dwResult; break;
                case 0x35: eventData.axis.Rz = result.dwResult; break;
                
                case 0x39: eventData.hat = result.dwResult; break;

                default:
                        if(usagePage == 0x09){
                                eventData.buttons |= result.dwResult << (usageID - 1);
                        }
                        break;
                }

                PrintByteValue(result.dwResult);
                E_Notify(rptSize, 0x80);
        }
        E_Notify(PSTR("\r\n"), 0x80);
}

JoystickDescParser::EventData JoystickDescParser::getEventData(){
        return eventData;
}

void JoystickDescParser::SetUsagePage(uint16_t page)
{
        ReportDescParserBase::SetUsagePage(page);

        usagePage = page;
}

template<typename T>
void JoystickDescParser::ZeroMemory(uint8_t len, T *buf) {
        for(uint8_t i = 0; i < len; i++)
                buf[i] = 0;
}

void JoystickReportParser::Parse(USBHID *hid, bool is_rpt_id __attribute__((unused)), uint8_t len, uint8_t *buf) {
        bool match = true;

        JoystickDescParser prs(len, buf);

        uint8_t ret = hid->GetReportDescr(0, &prs);

        eventData = prs.getEventData();
        
        Serial.println();
        Serial.println("------------------------------------------");

        Serial.print("X1: ");
        PrintHex<uint8_t > (eventData.axis.X, 0x80);
        Serial.print("\tY1: ");
        PrintHex<uint8_t > (eventData.axis.Y, 0x80);
        Serial.print("\tZ: ");
        PrintHex<uint8_t > (eventData.axis.Z, 0x80);
        Serial.print("\tRx: ");
        PrintHex<uint8_t> (eventData.axis.Rx, 0x80);
        Serial.println();
        Serial.print("\tRy: ");
        PrintHex<uint8_t > (eventData.axis.Ry, 0x80);
        Serial.print("\tRz: ");
        PrintHex<uint8_t> (eventData.axis.Rz, 0x80);
        Serial.println();

        PrintBin<uint32_t> (eventData.buttons, 0x80);
        Serial.println();
        E_Notify (eventData.hat, 0x80);
        Serial.println();

        Serial.println("------------------------------------------");

        if(ret)
                ErrorMessage<uint8_t > (PSTR("GetReportDescr-2"), ret);
}
