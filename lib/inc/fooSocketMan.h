//
// Created by Michael Uman on 10/28/20.
//

#ifndef SOCKETMANAGER_SOCKETMANAGER_H
#define SOCKETMANAGER_SOCKETMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <poll.h>
#include <functional>

//#define IPC_PACKET_HEADER_SIZE  4       // 4 bytes
//#define IPC_PACKET_HEADER_SOP   0xBEEF  // Start of Packet (SOP) Pattern

#define RECV_TIMEOUT_SEC 3     // recv() timeout in seconds
#define RECV_TIMEOUT_USEC 0    // recv() timeout in microseconds


namespace foo {
    namespace net {
        using dataPacket = std::vector<uint8_t>;
        using SocketReceivedCB = std::function<bool( const std::string &, int,
                                                     const dataPacket & )>;  // bool cb(name, fd, data)
        using SocketConnectCB = std::function<bool( const std::string &,
                                                    int )>;                       // bool cb(name, fd)

        class SocketManager;

        struct MacroHeader
        {
            uint16_t sop;
            uint16_t size;
        };

        const uint16_t IPC_PACKET_HEADER_SOP = 0xBEEF;
        const uint16_t IPC_PACKET_HEADER_SIZE = 4;

        class SocketHandler
        {
        public:
            SocketHandler( const std::string &name, const std::string &host, uint16_t port, bool server = false );

            virtual ~SocketHandler();

            bool onReceivePacket( const dataPacket &pkt );

            bool onReceivePacket( size_t index, const dataPacket &pkt );

            bool isOpen() const
            {
              return socket_fd_ != -1;
            };

            void setReceiveCB( SocketReceivedCB cb )
            {
              receivedCb_ = cb;
            };

            void setConnectCB( SocketConnectCB cb )
            {
              connectCb_ = cb;
            }

            void setDisconnectCB( SocketConnectCB cb )
            {
              disconnCb_ = cb;
            }

            bool readPacket( size_t client_id, dataPacket &data );

            bool writePacket( size_t client_id, dataPacket &data );

            bool readPacket( dataPacket &data ) const;

            bool writePacket( dataPacket &data ) const;

            std::string name() const
            { return name_; };

            uint16_t port() const
            { return port_; };

            int fd() const
            { return socket_fd_; };

            bool is_server( int fd ) const; // { return server_; };
            bool has_client( int fd, size_t &index );

            int client_count() const
            { return clientFds_.size(); };

            int client_fd( size_t index ) const
            { return clientFds_[index]; };

            int accept();

        private:

            std::string name_;
            std::string host_;
            bool server_ = false;
            uint16_t port_ = 0;
            int socket_fd_ = -1;

            SocketReceivedCB receivedCb_;
            SocketConnectCB connectCb_;
            SocketConnectCB disconnCb_;

            std::vector<int> clientFds_;
        };

        using SocketHandlerPtr = std::shared_ptr<SocketHandler>;
        using SocketHandlerPtrMap = std::map<std::string, SocketHandlerPtr>;

        class SocketManager
        {
        public:
            explicit SocketManager( const std::string &mgrName );

            virtual ~SocketManager();

            bool
            addSocketHandler( const std::string &name, const std::string &host, uint16_t port, bool server = false );

            bool addSocketHandler( const SocketHandlerPtr &handler );

            bool addReceiveCB( const std::string &name, SocketReceivedCB cb );

            bool addConnectCB( const std::string &name, SocketConnectCB cb );

            bool addDisconnectCB( const std::string &name, SocketConnectCB cb );

            SocketHandlerPtr getSocketByName( const std::string &name );

            SocketHandlerPtr getSocketByFd( int fd );

            bool open();

            void close();
//    bool    connect();

            bool processInbound();

        private:
            std::string name_;

            void allocate_pollfds();

            void addFd( int newfd );

            void delFd( int i );

            enum mgrState
            {
                CONFIG,
                OPEN,
            };
            SocketHandlerPtrMap recvHandlerMap_;
            SocketHandlerPtrMap connHandlerMap_;
            SocketHandlerPtrMap discHandlerMap_;

            mgrState state_ = CONFIG;
            pollfd *pollfds_ = nullptr;
            size_t fd_count_ = 0;
            size_t fd_size_ = 2;
        };

        struct subPacket
        {
            uint32_t type;
            uint32_t value;
        };

        std::string dump_vector( const dataPacket &vec );
    }
}

#endif //SOCKETMANAGER_SOCKETMANAGER_H
