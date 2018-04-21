#include<scif.h>
#include<fstream>
#include<vector>
#include<cstdlib>

using std::ofstream;
using std::vector;

int main( )
{
    int len = 61035 * 8192;
    scif_epd_t ep;
    scif_portID portid;
    uint16_t self;
#ifndef MIC
        ofstream cout( "output" );
        ep = scif_open( );
        if( ep == SCIF_OPEN_FAILED ) {
            cout << "scif open failed\n";
            return 1;
        }
        uint16_t nodeids[32];
        int nnodes = scif_get_nodeIDs( nodeids, 32, & self );
        for( int i = 0; i < nnodes; ++i ) {
            cout << "node id: " <<nodeids[i] << "\n";
        }
        cout << "I run on the host " << self << "\n";
        cout.flush();
        scif_bind( ep, 23954 );
        sleep(5);
        portid.node = 1;
        portid.port = 23954;
        scif_connect( ep, & portid );
        vector<int> buff(10, 1);
        scif_send(ep, &buff[0], buff.size() *sizeof(int), SCIF_SEND_BLOCK );


        double * ptr;
        int pagesize = sysconf(_SC_PAGESIZE);
        cout << "pagesize; " << pagesize << "\n";
        int ret = posix_memalign( (void**) & ptr, pagesize, len );
        if( ret ) {
            cout << "could not allocate\n";
            return 1;
        }
        cout << "sizeof long: " << sizeof(long) << "\n";
        if( SCIF_REGISTER_FAILED == scif_register( ep, ptr, len, (long)ptr, SCIF_PROT_READ | SCIF_PROT_WRITE, SCIF_MAP_FIXED ) ) {
            cout << "scif register failed\n";
            return 1;
        }
        void * remote_ptr;
        scif_recv( ep, & remote_ptr, sizeof(void*), SCIF_RECV_BLOCK);
        cout << "remote ptr: " << remote_ptr << "\n";
        for( int i = 0; i < len/8; ++i ) {
            ptr[i] = 43.892346;
        }
        vector<int> lengths;
        lengths.push_back( 61035 * 8192 );
        lengths.push_back( 1954 * 8192 );
        lengths.push_back( 977*8192 );
        lengths.push_back( 122 * 8192 );
        lengths.push_back( 12 * 8192 );
        lengths.push_back( 8192 );
        for( int i = 0; i < lengths.size( ); ++i ) {
            int ntrys = std::max( 1, 20000000 / lengths[ i ] );
            double time = 0;
            for( int trys = 0; trys < ntrys; ++trys ) {
                timespec t1, t2;
                clock_gettime( CLOCK_REALTIME, & t1);
                if( scif_writeto( ep, (long)ptr, lengths[i], (long)remote_ptr, 0 ) ) {
                    cout << "writeto failed\n";
                    cout.flush();
                    return 1;
                }
                scif_fence_signal( ep, (long)ptr, 435888, (long)remote_ptr, 435888, SCIF_FENCE_INIT_SELF | SCIF_SIGNAL_LOCAL | SCIF_SIGNAL_REMOTE );
                uint64_t volatile * p = (uint64_t volatile * )ptr;
                while( *p != 435888 ) usleep(100);
                clock_gettime( CLOCK_REALTIME, & t2);
                *p=0;
                time += (t2.tv_sec * 1E9 + t2.tv_nsec - t1.tv_sec*1E9 - t1.tv_nsec ) /1E9;
            }
            time /= ntrys;
            cout << "trys: " << ntrys << "\n";
            cout << "time: " << time << "\n";
            cout << "rate in byte/s: " << lengths[i]/ time << "\n";
            cout << lengths[i] << " " << lengths[i]/ time << "\n";
            cout.flush( );
        }
        cout << "completed\n";
        cout.flush( );

#else
        ofstream cout( "output2" );
        ep = scif_open( );
        if( ep == SCIF_OPEN_FAILED ) {
            cout << "scif open failed\n";
            return 1;
        }
        scif_get_nodeIDs( NULL, 0, & self );
        cout << "I run on the MIC " << self << "\n";
        cout.flush();
        scif_bind( ep, 23954 );
        scif_epd_t new_ep;
        scif_listen( ep, 4 );
        scif_accept( ep, &portid, & new_ep, SCIF_ACCEPT_SYNC );
        vector<int> buff(10, 0);
        scif_recv(new_ep, &buff[0], buff.size() *sizeof(int), SCIF_RECV_BLOCK );
        for( int i = 0; i < buff.size( ); ++i ) {
            cout << buff[i] << " ";
        }
        cout.flush();

        double * ptr;
        int pagesize = sysconf(_SC_PAGESIZE);
        int ret = posix_memalign( (void**) & ptr, pagesize, len );
        if( ret ) {
            cout << "could not allocate\n";
            return 1;
        }
        if( SCIF_REGISTER_FAILED == scif_register( new_ep, ptr, len, (long)ptr, SCIF_PROT_READ | SCIF_PROT_WRITE, SCIF_MAP_FIXED ) ) {
            cout << "scif register failed\n";
            return 1;
        }
        for( int i = 0; i < len/8; ++i ) {
            ptr[i] = 0;
        }
        long offset = 0;
        cout << "recv ptr: " << ptr << "\n";
        scif_send( new_ep, & ptr, sizeof(long), SCIF_SEND_BLOCK);
        cout.flush();
        uint64_t volatile * p = (uint64_t volatile * )ptr;
        sleep(60);
        while( *p != 435888 ) usleep(100);
        cout << "completed\n";
        cout << "recieved data: " << ptr[ 1 ] << "\n";
        cout << "recieved data: " << ptr[ len/8-1 ] << "\n";
        cout.flush( );
        scif_close( new_ep );

#endif
    scif_close( ep );
}

