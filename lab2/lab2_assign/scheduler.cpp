/*
    Code Written by Varun Deliwala 
    vd2298
    OS Lab 2 Scheduler
*/

#include <unistd.h> 
#include <cstdio>
#include <string.h>
#include <vector>
#include <algorithm>
#include <list>
#include <sstream>
#include <fstream>
#include <iostream>
#include <deque>

using namespace std;

// Enum Classes for transition and state
enum class Transition{ ToReady, ToRunning, ToBlocked, ToPreemption, ToFinished, BlockedToReady};


//Global Variables
vector<int> randomNumbers;
vector<int> RandomsList;
int RandomCount = 0;
bool callScheduler = false;



// Required data structures
struct SchedulerDetails {
    char type;
    int quantum = 10000; 
    int maxPriority = 4;
};

bool parseSchedulerOption(const char* optarg, SchedulerDetails& options) {
    
    options.type = optarg[0];
    char* rest = const_cast<char*>(optarg + 1); 
    char* token = strtok(rest, ":"); 
    if (token) {
        options.quantum = atoi(token); 
        token = strtok(nullptr, ":"); 
        if (token) {
            options.maxPriority = atoi(token); 
        }
    }
    return true; 
}

struct Process
{
    bool Preempted; 
    int ID;
    int ArrivalTime;
    int TotalCPUTime;
    int CPUBurst;
    int IOBurst;
    int StaticPriority;
    int DynamicPriority;
    int RemainingTime;
    int FinishTime;
    int TotalTurnAroundTime; // TAT = FT - AT
    int IOTime;
    int Waiting;
    int RemainingBurst;
    int StateStart;

    Process(int ID, int ArrivalTime, int TotalCPUTime, int CPUBurst, int IOBurst, int StaticPriority):
    ID(ID),ArrivalTime(ArrivalTime),TotalCPUTime(TotalCPUTime),CPUBurst(CPUBurst),IOBurst(IOBurst),
    StaticPriority(StaticPriority), DynamicPriority(StaticPriority - 1),RemainingTime(TotalCPUTime), 
    FinishTime(-1000), TotalTurnAroundTime(-1000), IOTime(0), Waiting(0), RemainingBurst(-1000), 
    StateStart(ArrivalTime), Preempted(false)
    {}
};

struct Event
{
    int time;
    Process* process;
    Transition transition;
    Event(int clockTime, Process* processInstance, Transition eventTransition) {
        time = clockTime;
        process = processInstance;
        transition = eventTransition;
    }
};
deque<Event*> EventQueue;
deque<Process *> RunQueue;

// Random function to read the random file and enter the entries into the RandomList
void RandomGen(const string& file) {
    int numberCount, number;
    ifstream infile(file);
    infile >> numberCount; 
    RandomsList.reserve(numberCount);
    while (infile >> number) 
    {
        RandomsList.push_back(number);
    } 
}

//Gives a new random number
int next(int value) 
{
    int tempRandom; 
    tempRandom = (RandomsList[RandomCount] % value);
    tempRandom += 1;    
    RandomCount = (++RandomCount) % RandomsList.size();
    return tempRandom;
}


struct DES {

    vector<Process*> processArray;
    int totalIOTime = 0;

    DES(const vector<Process*>& processes) : processArray(processes) {
        for (auto* p : processes) {
            // printf("Event is being added to process\n");
            addEvent(new Event(p->ArrivalTime, p, Transition::ToReady));
        }
    }

    ~DES() {
        for (auto* e : EventQueue) delete e;
    }

    void addEvent(Event* e) {
        auto it = find_if(EventQueue.begin(), EventQueue.end(), [e](const Event* other) { return other->time > e->time; });
        EventQueue.insert(it, e);
    }

    Event* nextEvent() {
        Event* e = EventQueue.front();
        EventQueue.pop_front();
        return e;
    }

    bool isEmpty() const { return EventQueue.empty(); }

    void deleteEventsForProcess(Process* process) {
        
        auto it = EventQueue.begin();
        for(; it != EventQueue.end(); ++it)
        {
            if ((*it)->process == process) {
                EventQueue.erase(it);
                return;
            }
        }
        EventQueue.erase(it);
        }
    

    int nextEventTime() const {
        return EventQueue.empty() ? -1 : EventQueue.front()->time;
    }

    int WaitTimeForNextEvent(Process* p) 
    {
        
        auto it = EventQueue.begin();
        for(; it != EventQueue.end(); ++it)
        {
            if ((*it)->process == p) {
                return (*it)->time;
            }
        }
        return -1;
    }
};

class Scheduler{  
    public:  
        int Quantum = 10000;
        int MaximumPriority = 4;
        
        virtual bool Breaking() {return false;}
        virtual void add(Process * Process) = 0;
        virtual Process* GetNew() = 0;
};

class FirstComeFirstServe: public Scheduler{
    public:
    void add(Process * Process) override 
    {
        Process->DynamicPriority = Process->StaticPriority - 1;
        RunQueue.push_back(Process);
    }
    Process* GetNew() override
    {
        Process* Process;
        if(RunQueue.empty()) 
        {
            return nullptr;
        }
        else
        {
            Process = RunQueue.front();
            RunQueue.pop_front();
            return Process;
        }
    } 
};

class LastComeFirstServe: public Scheduler{
    public:
    void add(Process * Process) override
    {
        Process->DynamicPriority = Process->StaticPriority - 1;
        RunQueue.push_back(Process);
    }
    Process* GetNew() override
    {
        Process* Process;
        if(RunQueue.empty()) 
        {
            return nullptr;
        }
        else
        {
            Process = RunQueue.back();
            RunQueue.pop_back();
            return Process;
        }
    } 
};


class ShortestRemainingTimeFirst: public Scheduler{
    public:
    void add(Process * Process) override
    {
        Process->DynamicPriority = Process->StaticPriority - 1;
        if(RunQueue.empty())
        {
            RunQueue.push_back(Process);
        }
        else
        {   
            auto it = RunQueue.begin();
            for (; it != RunQueue.end(); ++it) 
            {
                if (Process->RemainingTime < (*it)->RemainingTime) 
                {
                    break; // Found the insertion point
                }
            }
            RunQueue.insert(it, Process);
        }
    }
    
    // bool compareProcessRemainingTime(const Process* a, const Process* b) 
    // {
    //     return a->RemainingTime < b->RemainingTime;
    // }

    Process* GetNew() override
    {
        Process* Process;
        if(RunQueue.empty()) 
        {
            return nullptr;
        }
        else
        {
            Process = RunQueue.front();
            RunQueue.pop_front();
            return Process;
        }
    }
};

class RoundRobin: public Scheduler{
    public:
    RoundRobin(int Quantum)
    {
        this->Quantum = Quantum;
    }
    void add(Process* Process) override
    {
        Process->DynamicPriority = Process->StaticPriority - 1;
        RunQueue.push_back(Process);
    }
    
    Process* GetNew() override
    {
        Process* Process;
        if(RunQueue.empty()) 
        {
            return nullptr;
        }
        else
        {
            Process = RunQueue.front();
            RunQueue.erase(RunQueue.begin());
            return Process;
        }
    } 
};


class PriorityScheduler: public Scheduler {
public:
    public:
    vector<list<Process*>> activeQueue;
    vector<list<Process*>> expiredQueue;
    PriorityScheduler(int Quantum, int MaximumPriority = 4) 
    {
        this->Quantum = Quantum;
        this->MaximumPriority = MaximumPriority;
        activeQueue.resize(MaximumPriority);
        expiredQueue.resize(MaximumPriority);
    }

    void add(Process* p) {
        if (p->DynamicPriority < 0) {
            p->DynamicPriority = p->StaticPriority - 1;
            expiredQueue[p->DynamicPriority].push_back(p);
        } else {
            activeQueue[p->DynamicPriority].push_back(p);
        }
    }

    Process* GetNew() 
    {
        bool cond1 = true, cond2 = true;

        auto itExpired = expiredQueue.begin();
        while (itExpired != expiredQueue.end() && cond2) {  
            if (!itExpired->empty()) {
                cond2 = false;
            } else {
                ++itExpired;  
            }
        }

        auto itActive = activeQueue.begin();
        while (itActive != activeQueue.end() && cond1) {  
            if (!itActive->empty()) {
                cond1 = false;
            } else {
                ++itActive;  
            }
        }
        
        if (cond1 && cond2) {
            return nullptr;
        }
        else if (cond1) {
            activeQueue.swap(expiredQueue);
        }

        auto it = activeQueue.rbegin(); 
        while (it != activeQueue.rend()) {
            if (!it->empty()) {
                auto nextProcess = it->front();
                it->pop_front();
                return nextProcess;
            }
            ++it; 
        }

        return nullptr;
    }
};


class PreemptivePriorityScheduler: public PriorityScheduler{
    public:
        PreemptivePriorityScheduler(int Quantum, int MaximumPriority = 4) : PriorityScheduler(Quantum, MaximumPriority)
        {
            this->Quantum = Quantum;
            this->MaximumPriority = MaximumPriority;
        }
        bool Breaking()
        {
            bool Preemption = true;
            return Preemption;
        }
};
 


void simulation_loop(DES* des, Scheduler* scheduler) 
{
    
    Event* OngoingEvent = nullptr;
    Process* OngoingProcess = nullptr;   
    int IOStartTimestamp = 0, tempCB = 0, tempIOB = 0, numOfBlockedEvents = 0;

    while(!des->isEmpty())
    {
        OngoingEvent = des->nextEvent();
        int StateTime = OngoingEvent->time - OngoingEvent->process->StateStart;
        switch(OngoingEvent->transition)
        {
            case Transition::ToReady:
                OngoingEvent->process->StateStart = OngoingEvent->time;
                // printf("%d %d %d: CREATED -> READY\n", OngoingEvent->time, OngoingEvent->process->ID,
                //            StateTime);

                // Preemption implemented in the second phase of implemention
                if((OngoingProcess != nullptr) &&  scheduler->Breaking())
                {
                    if((OngoingEvent->process->DynamicPriority > OngoingProcess->DynamicPriority) && (des->WaitTimeForNextEvent(OngoingProcess) != OngoingEvent->time))
                    {
                        des->deleteEventsForProcess(OngoingProcess);
                        OngoingProcess->Preempted = true;
                        des->addEvent(new Event(OngoingEvent->time, OngoingProcess, Transition::ToPreemption));
                    }
                }
                // Preemption Implementation Over
                scheduler->add(OngoingEvent->process);
                callScheduler = true;
                break;
            
            case Transition::ToRunning:
                
                OngoingEvent->process->StateStart = OngoingEvent->time;
                OngoingProcess = OngoingEvent->process;
                OngoingEvent->process->Waiting += StateTime;
                // Preemption implemented in the second phase of implemention
                if(!OngoingEvent->process->Preempted)
                {
                    tempCB = next(OngoingEvent->process->CPUBurst);
                    tempCB = min(tempCB, OngoingEvent->process->RemainingTime);
                    OngoingEvent->process->RemainingBurst =  tempCB;
                    tempCB = OngoingEvent->process->RemainingBurst;
                }
                else{
                    tempCB = OngoingEvent->process->RemainingBurst;
                }
                // Preemption Implementation Over

                // printf("%d %d %d: READY -> RUNNG cb=%d rem=%d prio=%d\n",
                //            OngoingEvent->time, OngoingEvent->process->ID, StateTime, tempCB,
                //            OngoingEvent->process->RemainingTime,OngoingEvent->process->DynamicPriority);


                OngoingEvent->process->Preempted = false;

                if(scheduler->Quantum >= tempCB)
                {
                    if(tempCB >= OngoingEvent->process->RemainingTime)
                    {
                        //
                        des->addEvent(new Event(OngoingEvent->time + tempCB, OngoingEvent->process, Transition:: ToFinished));                
                    }
                    else
                    {
                        des->addEvent(new Event(OngoingEvent->time + tempCB,OngoingEvent->process, Transition::ToBlocked));
                    }
                }
                else
                {
                    des->addEvent(new Event((scheduler->Quantum + OngoingEvent->time), OngoingEvent->process, Transition:: ToPreemption));                                        
                }
                break;
            
            case Transition::ToPreemption:
                OngoingProcess = nullptr;

                

                // printf("%d %d %d: RUNNG -> READY cb=%d rem=%d prio=%d\n",
                //            OngoingEvent->time, OngoingEvent->process->ID, StateTime, tempCB,
                //            OngoingEvent->process->RemainingTime,OngoingEvent->process->DynamicPriority);

                OngoingEvent->process->RemainingBurst = OngoingEvent->process->RemainingBurst - StateTime;
                OngoingEvent->process->RemainingTime = OngoingEvent->process->RemainingTime - StateTime;
                OngoingEvent->process->StateStart = OngoingEvent->time;
                OngoingEvent->process->DynamicPriority = OngoingEvent->process->DynamicPriority - 1;
                scheduler->add(OngoingEvent->process);
                OngoingEvent->process->Preempted = true;
                callScheduler = true;
                break;        

            case Transition::ToBlocked:
                OngoingProcess = nullptr;
                OngoingEvent->process->RemainingTime -= StateTime;
                
                

                // printf("%d %d %d: RUNNG -> BLOCK  ib=%d rem=%d\n", OngoingEvent->time,
                //            OngoingEvent->process->ID, StateTime, tempIOB,
                //            OngoingEvent->process->RemainingTime);

                if(numOfBlockedEvents == 0) IOStartTimestamp = OngoingEvent->time;
                OngoingEvent->process->StateStart = OngoingEvent->time;
                tempIOB = next(OngoingEvent->process->IOBurst);
                des->addEvent(new Event(OngoingEvent->time + tempIOB, OngoingEvent->process, Transition::BlockedToReady));
                callScheduler = true;
                numOfBlockedEvents++;
                break;
            
            case Transition::BlockedToReady:
                
                numOfBlockedEvents -= 1;
                if(numOfBlockedEvents == 0) des->totalIOTime += (OngoingEvent->time - IOStartTimestamp);
               

                // printf("%d %d %d: BLOCK -> READY\n", OngoingEvent->time, OngoingEvent->process->ID,
                //            StateTime);

                OngoingEvent->process->StateStart = OngoingEvent->time;
                OngoingEvent->process->DynamicPriority = OngoingEvent->process->StaticPriority - 1;
                OngoingEvent->process->IOTime += StateTime;
                if((OngoingProcess != nullptr) &&  scheduler->Breaking())
                {
                    
                    if((OngoingEvent->process->DynamicPriority > OngoingProcess->DynamicPriority) && (des->WaitTimeForNextEvent(OngoingProcess) != OngoingEvent->time ))
                    {
                        // printf("The Event is being deleted");
                        des->deleteEventsForProcess(OngoingProcess);
                        OngoingProcess->Preempted = true;
                        des->addEvent(new Event(OngoingEvent->time, OngoingProcess, Transition::ToPreemption));
                    }
                }
                scheduler->add(OngoingEvent->process);
                callScheduler = true;
                break;

            // Ask about this. when I am trying to remove this, the answer is coming of by 1 time value 
            case Transition::ToFinished:
                // printf("%d %d %d: Done\n", OngoingEvent->time, OngoingEvent->process->ID, StateTime);
                OngoingEvent->process->RemainingTime -= StateTime;
                OngoingProcess = nullptr;
                OngoingEvent->process->FinishTime = OngoingEvent->time;
                OngoingEvent->process->TotalTurnAroundTime = OngoingEvent->time - OngoingEvent->process->ArrivalTime;
                callScheduler = true;
                break;
        }   

        if(callScheduler)
        {
            if(des->nextEventTime() == OngoingEvent->time)
            {
                continue;
            }
            else
            {
                callScheduler = false;
                if(OngoingProcess == nullptr)
                {
                    OngoingProcess = scheduler->GetNew();
                    if(OngoingProcess != nullptr)
                    {
                        des->addEvent(new Event(OngoingEvent->time, OngoingProcess, Transition::ToRunning));
                    }
                    
                }
            }
        }

    }
}

int main(int argc, char* argv[]) {

    int opt;
    SchedulerDetails schedulerOptions;
    Scheduler* scheduler;
    char* inputFile = nullptr;
    char* randomFile = nullptr;
    string name;

    while ((opt = getopt(argc, argv, "s:")) != -1) {
        switch (opt) {
            case 's':
                if (!parseSchedulerOption(optarg, schedulerOptions)) {
                    cerr << "Failed to parse scheduler options. Exiting.\n";
                    return EXIT_FAILURE;
                }
                break;
            default:
                cerr << "Usage: " << argv[0] << " -s <scheduler_option> <inputFile> <randomFile>\n";
                return EXIT_FAILURE;
        }
    }

    if (argc - optind < 2) {
        cerr << "Missing required arguments. Usage: " << argv[0] << " -s <scheduler_option> <inputFile> <randomFile>\n";
        return EXIT_FAILURE;
    } else {
        inputFile = argv[optind];
        randomFile = argv[optind + 1];
    }

    switch(schedulerOptions.type) {
        case 'F':
            scheduler = new FirstComeFirstServe();
            name = "FCFS";
            break;
        case 'L':
            scheduler = new LastComeFirstServe();
            name = "LCFS";
            break;
        case 'R':
            scheduler = new RoundRobin(schedulerOptions.quantum);
            name = "RR";
            break;
        case 'S':
            scheduler = new ShortestRemainingTimeFirst();
            name = "SRTF";
            break;
        case 'P': 
            scheduler = new PriorityScheduler(schedulerOptions.quantum, schedulerOptions.maxPriority);
            name = "PRIO";
            break;
        case 'E':
            scheduler = new PreemptivePriorityScheduler(schedulerOptions.quantum, schedulerOptions.maxPriority);
            name = "PREPRIO";
            break;
        default:
            cerr << "Unknown scheduler type: " << schedulerOptions.type << endl;
            exit(EXIT_FAILURE);
    }


    // For demonstration: print parsed options and file paths
    RandomGen(randomFile);
    
    vector<Process*> ProcessesList;
    string line;
    ifstream infile(inputFile);
    
    int ID = 0;
    string ArrivalTime;
    string TotalCPUTime; 
    string CPUBurst; 
    string IOBurst; 
    int StaticPriority;

    while (infile >> ArrivalTime >> TotalCPUTime >> CPUBurst >> IOBurst) {
        istringstream iss(line);
        iss >> ArrivalTime >> TotalCPUTime >> CPUBurst >> IOBurst;
        
        StaticPriority = next(schedulerOptions.maxPriority);
        Process* p = new Process(ID++, stoi(ArrivalTime), stoi(TotalCPUTime), stoi(CPUBurst), stoi(IOBurst), StaticPriority);
        ProcessesList.push_back(p);
    }

    for(int i = 0; i < ProcessesList.size(); i++)
    {
        // printf("The process added %d \n",ProcessesList[i]->ArrivalTime);
    }
    auto des = DES(ProcessesList);

    simulation_loop(&des, scheduler);
    
    
    //Printing stuff starts here
    
    if (schedulerOptions.quantum != 10000) {
        printf("%s ", name.c_str());
        cout << "" << schedulerOptions.quantum << endl;
    }
    else{
        printf("%s\n", name.c_str());
    }

    int FinalTime;
    double CPUUtilization;
    double TotalTurnAroundTime;
    double TotalWaitingTime;
    int numberOfProcesses = des.processArray.size();
    double ThroughPut;        
    double IOUtilization;

    for (auto p : des.processArray) {
        if(p->FinishTime > FinalTime) FinalTime = p->FinishTime;
        CPUUtilization += p->TotalCPUTime;
        TotalTurnAroundTime += p->TotalTurnAroundTime; 
        TotalWaitingTime += p->Waiting;
        
        printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n", p->ID, p->ArrivalTime,
               p->TotalCPUTime, p->CPUBurst, p->IOBurst, p->StaticPriority, p->FinishTime,
               p->TotalTurnAroundTime, p->IOTime, p->Waiting);
    }
    
    CPUUtilization = 100.0*(CPUUtilization/FinalTime);
    TotalTurnAroundTime = TotalTurnAroundTime/numberOfProcesses;
    TotalWaitingTime = TotalWaitingTime/numberOfProcesses;
    ThroughPut = 100.0*(double(numberOfProcesses) / double(FinalTime));
    IOUtilization = 100.0*(double(des.totalIOTime) / double(FinalTime));
    printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", FinalTime, CPUUtilization,
           IOUtilization, TotalTurnAroundTime, TotalWaitingTime, ThroughPut);


}