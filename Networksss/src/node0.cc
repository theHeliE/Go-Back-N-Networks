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

void Node0::initialize()
{
    // TODO - Generated method body
    string msg;
    cout << "Enter a word" << endl;
    cin >> msg;

    string framedMsg = Framing(msg);
    cout << "framed message is" << framedMsg << endl;
}

void Node0::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
    EV << "message Received at Node 0";
}
