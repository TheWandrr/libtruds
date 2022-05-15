# libtruds

This is a library initially developed on a Raspberry Pi 3B+ that intends to facilitate the connection to (full size) Ford Transit vans using the Unified Diagnostic Service (UDS) protocol.

It is a work in progress and would benefit from a lot of cleanup and directional focus.

# Interface

Communication uses linux SocketCAN, so anything that can be added to the system and show up as a CAN device should work fine with this code. Initial development and testing used a MCP2515 based CAN module, for which there is kernel support on the Raspberry Pi. Some potential issues with the volume of network traffic have been observed with this, as the hardware ID filtering has not been implemented and thus must be performed in software.

# Building and Installing

    > make
    > sudo make install
    
    *** Note that there may be additional packages to install. Let me know and I'll add them here to help others in the future.

# Purpose and Scope

Due to variations in the way vehicle manufacturers have implemented the standard, plus the excrusiating lack of published information in this area, the focus of this project has been on a specific vehicle make and model. Many of the concepts are applicable to other vehicles but some reorganization would be required to make this happen in the future. Testing has been limited to a North American 2018 model year 3.8L gasoline Ford Transit 250. It is with the hope that many other combinations can be validated that this project is being made available to the public.

This is a library, so only minimal test code is included. No user interface currently exists, but if this code reaches a stage of maturity then such a creation would be of benefit to the community.
