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

#include "coordinator.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem> // C++17 or later
using namespace std;

Define_Module(Coordinator);

void Coordinator::initialize()
{

    // Open the file
    ifstream infile("Textfiles/coordinator.txt");
    if (!infile.is_open())
    {
        cerr << "Error opening file: Textfiles/coordinator.txt" << endl;
        return;
    }

    // Read the file line by line
    string line;
    while (getline(infile, line))
    {
        // Parse the line to extract time and node_id
        istringstream iss(line);
        if (!(iss >> node_id >> starting_time))
        {
            cerr << "Error reading line: " << line << endl;
            continue;
        }
    }

    // Close the file
    infile.close();

    // Schedule a self-message to handle the delay
    cMessage *selfMsg = new cMessage("selfMsg");
    scheduleAt(simTime() + starting_time, selfMsg);

    // Store the node_id for use in handleMessage
    this->node_id = node_id;
}

void Coordinator::handleMessage(cMessage *msg)
{
    // check if the message is the self-message
    if (strcmp(msg->getName(), "selfMsg") == 0)
    {
        // If node_id == 0, then send a message to node 0 else node 1
        string sent = "Coordinator sent message to node ";
        if (node_id == 0)
        {
            sent += "0";
            cMessage *sent_msg_to_node_0 = new cMessage(sent.c_str());
            send(sent_msg_to_node_0, "out", 0);
            EV << "Coordinator sent message to node 0" << endl;
        }
        else
        {
            sent += "1";
            cMessage *sent_msg_to_node_1 = new cMessage(sent.c_str());
            send(sent_msg_to_node_1, "out", 1);
            EV << "Coordinator sent message to node 1" << endl;
        }
    }

    // Clean up the self-message
    delete msg;
}
