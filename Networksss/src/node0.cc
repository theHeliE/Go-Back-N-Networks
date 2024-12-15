//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "node0.h"

Define_Module(Node0);
using namespace std;

string Node0::Framing(const string &data)
{
    string framedData;

    // START
    framedData += '$';

    for (char c : data)
    {
        if (c == '$' || c == '/')
        {
            // add ESC
            framedData += '/';
        }
        // add Data
        framedData += c;
    }

    // END
    framedData += '$';

    return framedData;
}

string Node0::Deframing(const string &data)
{
    // define a string to store the deframed data as well as a
    // when the data is deframed, first and last character is removed (flag characters)
    // then when a '/' is found, the next character is added to the deframed data
    // this is because the character after the '/' is the original character
    string deframedData;

    for (int i = 1; i < data.length() - 1; i++)
    {
        // if the character is a '/', skip it and go to the next character
        if (data[i] == '/')
        {
            i++;
        }
        deframedData += data[i];
    }

    return deframedData;
}

bool Node0::ErrorDetection(MyMessage_Base *msg)
{

    // Get the Payload and create the Variable that stores the XOR value
    string s = std::string(msg->getM_Payload());
    bitset<8> xorval(0);

    // XOR the payload
    for (int i = 0; i < s.size(); i++)
    {
        bitset<8> x(s[i]);
        xorval = xorval ^ x;
    }

    // XOR the trailer
    xorval ^= bitset<8>(msg->getM_Trailer());

    // Check if the XOR value is not 0 then no error detected else error detected

    return xorval != 0;
}

void Node0::initialize()
{
    // TODO - Generated method body
    string msg;
    cout << "Enter a word" << endl;
    cin >> msg;

    string framedMsg = Framing(msg);
    cout << "framed message is :" << framedMsg << endl;
    cout << "deframed message is :" << Deframing(framedMsg) << endl;

    MyMessage_Base *msg_test = new MyMessage_Base();
    bitset<8> xorval(0);
    for (int i = 0; i < framedMsg.size(); i++)
    {
        bitset<8> x(framedMsg[i]);
        xorval = xorval ^ x;
    }
    char parity = (char)xorval.to_ulong();

    // Manipulate after framing :
    //  Access the first character and modify its fourth bit
    //unsigned char firstChar = framedMsg[0];
   // firstChar ^= (1 << 3);
    //framedMsg[0] = firstChar;

    const char *original_message = framedMsg.c_str();

    msg_test->setSeq_Num(0);
    msg_test->setM_Type(2);
    msg_test->setACK_Num(0);
    msg_test->setNACK_Num(0);
    msg_test->setM_Payload(original_message);
    msg_test->setM_Trailer(parity);

    if (ErrorDetection(msg_test))
    {
        cout << "Error Detected" << endl;
    }
    else
    {
        cout << "No Error Detected" << endl;
    }
}

void Node0::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}
