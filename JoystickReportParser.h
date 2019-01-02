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
#if !defined(__JOYSTICKREPORTPARSER_H__)
#define __JOYSTICKREPORTPARSER_H__

#include "hidescriptorparser.h"

class JoystickDescParser : public ReportDescParserBase {
public:
        struct EventData{
                struct Axis{
                        uint8_t X, Y, Z, Rx, Ry, Rz;
                } axis = {0};
                
                uint32_t buttons = {0};
                uint8_t hat = {0};
        };

private:
        EventData eventData;
   

        static const uint16_t usageBufferSize = 64; // event buffer length
        uint8_t usage[usageBufferSize] = {0}; 
        uint8_t usageIndex;

        uint8_t rptId; // Report ID
        uint8_t useMin; // Usage Minimum
        uint8_t useMax; // Usage Maximum
        uint8_t fieldCount; // Number of field being currently processed

        void OnInputItem(uint8_t itm); // Method which is called every time Input item is found

        uint8_t *pBuf; // Report buffer pointer
        uint8_t bLen; // Report length
        
        uint8_t bits_of_byte;
protected:
        // Method should be defined here if virtual.
        virtual uint8_t ParseItem(uint8_t **pp, uint16_t *pcntdn);

        template<typename T>
        void ZeroMemory(uint8_t len, T *buf);
public:
        EventData getEventData();

        JoystickDescParser(uint16_t len, uint8_t *pbuf) :
        ReportDescParserBase(), usageIndex(0), rptId(0), useMin(0), useMax(0), fieldCount(0), pBuf(pbuf), bLen(len), bits_of_byte(8) {
        };
};

class JoystickReportParser : public HIDReportParser {
public:
        // Method should be defined here if virtual.
        virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);

private:        
        JoystickDescParser::EventData eventData;
};

#endif // __HIDDESCRIPTORPARSER_H__
