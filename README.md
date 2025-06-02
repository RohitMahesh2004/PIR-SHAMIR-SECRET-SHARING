# PIR using Shamir Secret SharingThis repository contains a C++ implementation of a Private Information Retrieval system using Shamir Secret Sharing and secure dot product.

## Components

- **db-upload-client.cpp**: Uploads secret-shared DB to servers.
- **pir-user-client.cpp**: User queries the servers privately.
- **pir-server.cpp**: Server processes DB shares and query shares.

## Usage

1. Start all three servers
2. Run db-upload-client
3. Run pir-user-client to retrieve DB[i] privately.
