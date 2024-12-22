package networksss;

import ned.IdealChannel;


@license(LGPL);
//
// TODO documentation
//
network Network
{
    parameters:
        int WS = default(4); // Window size
        int TO = default(7); // timeout interval in seconds
        double PT = default(0.5); // The sender’s and receivers’ processing time for each frame
        double TD = default(1.0); // Transmission delay for any frame
        double ED = default(4.0); // Error delay for any frame
        double DD = default(0.1); // The channel’s duplication delay before sending the second version
        double LP = default(0.1); // ACK/NACK frame loss probability, by default = 10%.
   		@display("bgb=269,259");
    submodules:
        node0: Node0 {
            @display("p=29,68");
        }
        node1: Node1 {
            @display("p=183,57");
        }
        coordinator: Coordinator {
            @display("p=114,179");
        }
    connections:
        node0.out --> IdealChannel --> node1.in[0];
        node1.out --> IdealChannel --> node0.in[0];
        coordinator.out[1] --> IdealChannel --> node1.in[1];
        coordinator.out[0] --> IdealChannel --> node0.in[1];
}

