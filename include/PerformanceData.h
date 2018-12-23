//
// Created by Bowen on 10/9/18.
//

#ifndef CPISYNCLIB_PERFORMANCEDATA_H
#define CPISYNCLIB_PERFORMANCEDATA_H

#include "GenSync.h"
#include "kshinglingSync.h"
#include "StrataEst.h"
#include "ForkHandle.h"
#include <fstream>
#include <omp.h>
#include <numeric>
#include <mutex>

class PlotRegister { // Export Data into a txt file for external code to graph
public:
    PlotRegister(string _title, vector<string> _labels);// init - open a file with a title and labels

    ~PlotRegister();

    void add(vector<string> datum); // add to data
    void update(); // bulk insert to what is in the data, clear data, close file after.
private:

    void init();

    string title; // title of the graph, used as file name
    vector<string> labels; //label including units
    vector<vector<string>> data; // Number of
};




static void kshingle3D(GenSync::SyncProtocol setReconProto, vector<int> edit_distRange,
                       vector<int> str_sizeRange, int tesPts, int confidence, string (*stringInput)(int), int portnum) {
   string protoName, str_type;
   if (GenSync::SyncProtocol::CPISync == setReconProto) protoName = "CPISync";
   if (GenSync::SyncProtocol::IBLTSyncSetDiff == setReconProto) protoName = "IBLTSyncSetDiff";
   if (GenSync::SyncProtocol::InteractiveCPISync == setReconProto) protoName = "InteractiveCPISync";

   if (*stringInput == randAsciiStr) str_type = "RandAscii";
   if (*stringInput == randSampleTxt) str_type = "RandText";
   if (*stringInput == randSampleCode) str_type = "RandCode";

   PlotRegister plot = PlotRegister("kshingle " + protoName + " " + str_type,
                                    {"Str Size", "Edit Diff", "Comm (bits)", "Time Set(s)", "Time Str(s)",
                                     "Space (bits)", "Set Recon True", "Str Recon True"});
   //TODO: Separate Comm, and Time, Separate Faile rate.

   for (int str_size : str_sizeRange) {
      cout << to_string(str_size) << endl;
      edit_distRange.clear();
      for (int i = 1; i <= tesPts; ++i) edit_distRange.push_back((int) ((str_size * i) / 400));
      for (int edit_dist : edit_distRange) {

//            int shingle_len = ceil(log2(str_size));
         int shingle_len = ceil(log10(str_size));

         for (int con = 0; con < confidence; ++con) {
            try {

               GenSync Alice = GenSync::Builder().
                       setProtocol(setReconProto).
                       setStringProto(GenSync::StringSyncProtocol::kshinglingSync).
                       setComm(GenSync::SyncComm::socket).
                       setPort(portnum).
                       setShingleLen(shingle_len).
                       build();


               DataObject *Alicetxt = new DataObject(stringInput(str_size));

               Alice.addStr(Alicetxt, false);
               GenSync Bob = GenSync::Builder().
                       setProtocol(setReconProto).
                       setStringProto(GenSync::StringSyncProtocol::kshinglingSync).
                       setComm(GenSync::SyncComm::socket).
                       setPort(portnum).
                       setShingleLen(shingle_len).
                       build();

               DataObject *Bobtxt = new DataObject(randStringEdit((*Alicetxt).to_string(), edit_dist));

// Flag true includes backtracking, return false if backtracking fails in the alloted amoun tog memory
               auto str_s = clock();
//                    struct timespec start, finish;
//                    double str_time;

//                    clock_gettime(CLOCK_MONOTONIC, &start);

               bool success_StrRecon = Bob.addStr(Bobtxt, true);

//                    clock_gettime(CLOCK_MONOTONIC, &finish);
//
//                    str_time = (finish.tv_sec - start.tv_sec);
//                    str_time += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
               double str_time = (double) (clock() - str_s) / CLOCKS_PER_SEC;


               multiset<string> alice_set;
               for (auto item : Alice.dumpElements()) alice_set.insert(item->to_string());
               multiset<string> bob_set;
               for (auto item : Bob.dumpElements()) bob_set.insert(item->to_string());

               int success_SetRecon = multisetDiff(alice_set,
                                                   bob_set).size();// separate set recon success from string recon
//                    auto bobtxtis = Alice.dumpString()->to_string();
//                    bool success_SetRecon = ((*Bobtxt).to_string() ==
//                                             Alice.dumpString()->to_string()); // str Recon is deterministic, if not success , set recon is the problem
               forkHandleReport report = forkHandle(Alice, Bob, false);

               plot.add({to_string(str_size), to_string(edit_dist), to_string(report.bytesTot),
                         to_string(report.CPUtime), to_string(str_time),
                         to_string(Bob.getVirMem(0)), to_string(success_SetRecon), to_string(success_StrRecon)});

               delete Alicetxt;
               delete Bobtxt;
            } catch (std::exception) {
               cout << "we failed once" << endl;
            }
         }
      }
      plot.update();
   }
}

static std::mutex mtx;
static void setsofcontent(GenSync::SyncProtocol setReconProto, vector<int> edit_distRange,
                          vector<int> str_sizeRange, int tesPts, int confidence, string (*stringInput)(int), int portnum) {

   string protoName, str_type;
   if (GenSync::SyncProtocol::IBLTSyncSetDiff == setReconProto) protoName = "IBLTSyncSetDiff";
   if (GenSync::SyncProtocol::InteractiveCPISync == setReconProto) protoName = "InteractiveCPISync";

   if (*stringInput == randAsciiStr) str_type = "RandAscii";
   if (*stringInput == randSampleTxt) str_type = "RandText";
   if (*stringInput == randSampleCode) str_type = "RandCode";

   PlotRegister plot = PlotRegister("Sets of Content " + protoName + " " + str_type,
                                    {"Str Size", "Edit Diff", "Comm (bits)", "Time Set(s)", "Time Str(s)",
                                     "Space (bits)", "Set Recon True", "Str Recon True"});
   //TODO: Separate Comm, and Time, Separate Faile rate.

   for (int str_size : str_sizeRange) {
      cout << to_string(str_size) << endl;
      edit_distRange.clear();
      for (int i = 1; i <= tesPts; ++i) edit_distRange.push_back((int) ((str_size * i) / 400));
      for (int edit_dist : edit_distRange) {

         for (int con = 0; con < confidence; ++con) {
             mtx.lock();
            try {

cout<<"I am "<< str_type<<endl;
               size_t terminalStrSize = (log10(str_size))*50;
               GenSync Alice = GenSync::Builder().
                       setStringProto(GenSync::StringSyncProtocol::SetsOfContent).
                       setProtocol(setReconProto).
                       setComm(GenSync::SyncComm::socket).
                       setTerminalStrSize(200).
                       setNumPartitions(10).
                       setlvl(1).
                       setPort(portnum).
                       build();


               DataObject *Alicetxt = new DataObject(stringInput(str_size));

               Alice.addStr(Alicetxt, false);

               GenSync Bob = GenSync::Builder().
                       setStringProto(GenSync::StringSyncProtocol::SetsOfContent).
                       setProtocol(setReconProto).
                       setComm(GenSync::SyncComm::socket).
                       setTerminalStrSize(200).
                       setNumPartitions(10).
                       setlvl(1).
                       setPort(portnum).
                       build();


               DataObject *Bobtxt = new DataObject(randStringEditBurst((*Alicetxt).to_string(), edit_dist));

               Bob.addStr(Bobtxt, false);



               forkHandleReport report = forkHandle(Alice, Bob, false);

               bool success_StrRecon = (Alice.dumpString()->to_string() == Bobtxt->to_string());


               plot.add({to_string(str_size), to_string(edit_dist), to_string(report.bytesTot+report.bytesRTot),
                         to_string(report.totalTime), to_string(success_StrRecon)});

               delete Alicetxt;
               delete Bobtxt;
            } catch (std::exception) {
               cout << "we failed once" << endl;
            }
             mtx.unlock();
         }
      }
      plot.update();
   }

}



static void strataEst3D(pair<size_t, size_t> set_sizeRange, int tesPts, int confidence) {
   int set_sizeinterval = floor((set_sizeRange.second - set_sizeRange.first) / tesPts);

   PlotRegister plot = PlotRegister("Strata Est",{"Set Size","Set Diff","Est"});

//#if __APPLE__
//    confidence /=omp_get_max_threads();
//#pragma omp parallel num_threads(omp_get_max_threads())
//#endif
   for (int set_size = set_sizeRange.first; set_size <= set_sizeRange.second; set_size += set_sizeinterval) {
      (set_size < set_sizeRange.first + (set_sizeRange.second-set_sizeRange.first)/2) ? confidence : confidence=5;
      cout<<"Current Set Size:"+to_string(set_size)<<endl;
      printMemUsage();
      int top_set_diff = set_size / 10;
      int set_diffinterval = floor((top_set_diff) / tesPts);

      for (int set_diff = 0; set_diff <= top_set_diff; set_diff += set_diffinterval) {

//            if (set_size>set_sizeRange.second/2)confidence = 100;
//            printMemUsage();
         //printMemUsage();
//#if __APPLE__
//#pragma omp critical
//#endif
         for (int conf = 0; conf < confidence; ++conf) {

            StrataEst Alice = StrataEst(sizeof(DataObject));
            StrataEst Bob = StrataEst(sizeof(DataObject));
//#if __APPLE__
//#pragma omp parallel firstprivate(Alice,Bob)
//#endif
            for (int j = 0; j < set_size; ++j) {
               auto tmp = randZZ();
               if (j < set_size - ceil(set_diff / 2)) Alice.insert(new DataObject(tmp));

               if (j >= ceil(set_diff / 2)) Bob.insert(new DataObject(tmp));
            }
            plot.add({to_string(set_size), to_string(set_diff), to_string((Alice -= Bob).estimate())});
         }
         //printMemUsage();

      }
//#if __APPLE__
//#pragma omp critical
//#endif
      plot.update();
   }
}






#endif //CPISYNCLIB_PERFORMANCEDATA_H