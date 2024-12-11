package networksss;

import ned.IdealChannel;


@license(LGPL);
//
// TODO documentation
//
network Network
{
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

