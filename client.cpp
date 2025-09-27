/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Andrew Marshall
	UIN: 733005012
	Date: 9/16/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <iomanip>
#include <chrono>
using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = -1.0;
	int e = 1;
	bool newChan = false;
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'c':
				newChan = true;
				break;
		}
	}

	char *serverCMD[] = { (char *)"./server", nullptr };
	
	pid_t pid = fork();
	 if (pid == -1) {
        cerr << "fork failed\n";
        return 1;
    }
	if(pid == 0){
		execvp(serverCMD[0], serverCMD);
	}else{

	FIFORequestChannel controlChan("control", FIFORequestChannel::CLIENT_SIDE);
	FIFORequestChannel* chan = &controlChan;
	if(newChan){
		MESSAGE_TYPE newM = NEWCHANNEL_MSG;
		controlChan.cwrite(&newM, sizeof(MESSAGE_TYPE));

        char newchanname[100];
        controlChan.cread(newchanname, sizeof(newchanname));

        cout << "New channel created: " << newchanname << endl;

        chan = new FIFORequestChannel(newchanname, FIFORequestChannel::CLIENT_SIDE);
	}
	// example data point request
    // char buf[MAX_MESSAGE]; // 256
    // datamsg x(1, 0.0, 1);
	
	// memcpy(buf, &x, sizeof(datamsg));
	// chan.cwrite(buf, sizeof(datamsg)); // question
	// double reply;
	// chan.cread(&reply, sizeof(double)); //answer
	// cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	
	if(!filename.empty()){
		string outpath =  "received/" + filename;
		// get file length
		filemsg fm(0, 0);

		int len = sizeof(filemsg) + (filename.size() + 1);
		char *buf = new char[len];
		memcpy(buf, &fm, sizeof(filemsg));
		strcpy(buf + sizeof(filemsg), filename.c_str());
		chan->cwrite(buf, len); // I want the file length;
		__int64_t filesize;
        chan->cread(&filesize, sizeof(__int64_t));
		//request file in chunks
		ofstream outfile(outpath, ios::binary);
		__int64_t offset = 0;
        int max_chunk = MAX_MESSAGE;
		auto start = chrono::high_resolution_clock::now();
		while (offset < filesize) {
            __int64_t remaining = filesize - offset;
			//this checks if the remaining amount is smaller than max chunk size
            __int64_t chunk_size = min((__int64_t)max_chunk, remaining);

            filemsg* fm_chunk = (filemsg*) buf;
            fm_chunk->offset = offset;
            fm_chunk->length = chunk_size;

            chan->cwrite(buf, len);
            char* response = new char[chunk_size];
            int nbytes = chan->cread(response, chunk_size);
            outfile.write(response, nbytes);
            delete[] response;

            offset += nbytes;
        }
		outfile.close();
        delete[] buf;
		auto end = chrono::high_resolution_clock::now();
		double duration = chrono::duration<double>(end - start).count();
		cout << "File transfer complete: " << outpath << endl;
		cout << "Time taken: " << duration << " seconds" << endl;
	}else if (t >= 0.0) {
        // ---- Single data point request ----
        datamsg x(p, t, e);
        chan->cwrite(&x, sizeof(datamsg));
        double reply;
        chan->cread(&reply, sizeof(double));
        cout << "For person " << p 
             << ", at time " << t 
             << ", the value of ecg " << e 
             << " is " << reply << endl;
    }else{
		string outpath = "received/x1.csv";
		ofstream ofs(outpath);
		for (int i = 0; i < 1000; i++) {
            double time = i * 0.004; // sampling interval
            datamsg req1(p, time, 1);
            chan->cwrite(&req1, sizeof(datamsg));
            double ecg1; 
            chan->cread(&ecg1, sizeof(double));

            datamsg req2(p, time, 2);
            chan->cwrite(&req2, sizeof(datamsg));
            double ecg2; 
            chan->cread(&ecg2, sizeof(double));

            // ofs << fixed << setprecision(3) << time << ","
            //     << setprecision(6) << ecg1 << ","
            //     << ecg2 << "\n";
			ofs << time << "," << ecg1 << "," << ecg2 << "\n";
        }
		ofs.close();
        cout << "Collected first 1000 data points into " << outpath << endl;
	}

    // sending a non-sense message, you need to change this
	// filemsg fm(0, 0);
	// string fname = "teslkansdlkjflasjdf.dat";
	
	// int len = sizeof(filemsg) + (fname.size() + 1);
	// char* buf2 = new char[len];
	// memcpy(buf2, &fm, sizeof(filemsg));
	// strcpy(buf2 + sizeof(filemsg), fname.c_str());
	// chan.cwrite(buf2, len);  // I want the file length;

	// delete[] buf2;
	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan->cwrite(&m, sizeof(MESSAGE_TYPE));
	if (newChan) {
        delete chan;  // close the new channel
        controlChan.cwrite(&m, sizeof(MESSAGE_TYPE)); // close control too
    }
	}
}
