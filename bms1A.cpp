/* 
    Projekt: BMS 2015, projekt 1
    Jméno: Modulace QPSK
    Autor: Ondřej Šlampa, xslamp01@stud.fit.vutbr.cz
*/

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>

#include "sndfile.hh"

//nosná frekvence
#define CARRIER_FREQ 1000.0
//vzorkovací frekvence
#define SAMPLE_RATE 18000.0
//délka symbolu v periodách nosné frekvence
#define SYMBOL_LENGTH (1.0)
//délka symbolu ve vzorcích
#define SYMBOL_SAMPLES (int)(SYMBOL_LENGTH*SAMPLE_RATE/CARRIER_FREQ)
//počet kanálů
#define CHANELS 1
#define FORMAT (SF_FORMAT_WAV | SF_FORMAT_PCM_24)
//amplituda
#define AMPLITUDE (1.0 * 0x7F000000)
//fázové posuny
#define SHIFT00 (3.0*M_PI_4)
#define SHIFT01 (1.0*M_PI_4)
#define SHIFT10 (5.0*M_PI_4)
#define SHIFT11 (7.0*M_PI_4)

using namespace std;

/*
    Vypočítá hodnotu nosné frekvence v čase x a s fázovým posunem shift.
    x čas
    shift fázový posun
*/
inline double carrier_wave(double x, double shift){
    return AMPLITUDE*sin(x*CARRIER_FREQ*2*M_PI+shift);
}

/*
    Provede modulaci dat ze vstupu do na výstup podle QPSK.
    input vstup
    output výstup
    counter počet zapsaných symbolů ve výstupním souboru před zavoláním
    této funkce
    return celkový počet zapsaných symbolů ve výstupním souboru nebo -1 pokud
    došlo k chybě
*/
int modulate(istream& input, SndfileHandle& output, int counter){
    //bity ze vstupního souboru
    char a=0;
    char b=0;
    //fázový posun
    double shift=0.0;
    int *buffer = new int[SYMBOL_SAMPLES];
    
    for(;;){
        //načtení bitů
        a=input.get();
        b=input.get();
        
        //podle kombinace bitů se určí fázový posun
        if(a=='0'&&b=='0'){
            shift=SHIFT00;
        }
        else if(a=='0'&&b=='1'){
            shift=SHIFT01;
        }
        else if(a=='1'&&b=='0'){
            shift=SHIFT10;
        }
        else if(a=='1'&&b=='1'){
            shift=SHIFT11;
        }
        //pokud jsou oba bity -1 nebo je první nový řádek
        //a druhý -1 - konec souboru
        else if((a==-1&&b==-1)||(a=='\n'&&b==-1)){
            break;
        }
        //jiné hodnoty bitů - chyba vstupního souboru
        else{
            cerr<<"Input file error."<<endl;
            return -1;
        }
        
        //modulace jednoho symbolu
        for (int i=0; i<SYMBOL_SAMPLES; i++){
            buffer[i]=carrier_wave(counter*SYMBOL_LENGTH/CARRIER_FREQ+i/SAMPLE_RATE, shift);
        }
        
        //zápis symbolu
        output.write(buffer,SYMBOL_SAMPLES);
        counter++;
    }
    
    delete [] buffer;
    return counter;
}

/*
    Hlavní funkce. 
*/
int main(int argc, char** argv) {
    //kontrola počtu parametrů
    if(argc!=2){
        cerr<<"Wrong number of arguments."<<endl;
        return EXIT_FAILURE;
    }
    
    string input_filename(argv[1]);
    if(input_filename.length()<5||(input_filename.substr(input_filename.length()-4,4)!=".txt")){
        cerr<<"Incorrect input filename."<<endl;
        return EXIT_FAILURE;
    }
    
    //synchronizační frekvence
    istringstream sync("00110011");
    
    string output_filename=input_filename.substr(0,input_filename.length()-4)+".wav";
    SndfileHandle output;
    output=SndfileHandle(output_filename, SFM_WRITE, FORMAT, CHANELS, SAMPLE_RATE);
    
    //počet zapsaných symbolů
    int cntr=0;
    
    //modulace synchronizační frekvence
    cntr=modulate(sync,output,cntr);
    
    if(cntr==-1){
        cerr<<"Synchronization problem."<<endl;
        return EXIT_FAILURE;
    }
    
    ifstream input;
    input.open(argv[1]);
    
    //modulace vstupního souboru
    cntr=modulate(input,output,cntr);
    
    input.close();
    if(cntr==-1){
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

