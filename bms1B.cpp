/* 
    Projekt: BMS 2015, projekt 1
    Jméno: Demodulace QPSK
    Autor: Ondřej Šlampa, xslamp01@stud.fit.vutbr.cz
*/

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <math.h>

#include "sndfile.hh"

//nosná frekvence
#define CARRIER_FREQ 1000.0
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
    return hodnota nosné frekvence
*/
inline double carrier_wave(double x, double shift){
    return AMPLITUDE*sin(x*CARRIER_FREQ*2*M_PI+shift);
}

/*
    Zjistí jetli jsou si obě hodnoty přibližně rovny.
    a první hodnota
    b druhá hodnota
    return jsou si hodnoty přibližně rovny
*/
inline bool nearly_equal(int a, int b){
    return abs(a-b)<(AMPLITUDE/1000);
}

/*
    Zjistí fázový posun vzorku value v čase time. Při sporném určení se použije
    hodnota předchozího fázového posunu.
    time čas
    value hodnota vzorku
    previous_shift předchozí fázový posun
    return fázový posun současného vzorku
*/
double get_shift(double time, double value, double previous_shift){
    //počitadlo idenfikovaných fázových posunů
    int cntr=0;
    //fázový posun
    double shift=0.0;
    if(nearly_equal(carrier_wave(time,SHIFT00),value)){
        cntr++;
        shift=SHIFT00;
    }
    if(nearly_equal(carrier_wave(time,SHIFT01),value)){
        cntr++;
        shift=SHIFT01;
    }
    if(nearly_equal(carrier_wave(time,SHIFT10),value)){
        cntr++;
        shift=SHIFT10;
    }
    if(nearly_equal(carrier_wave(time,SHIFT11),value)){
        cntr++;
        shift=SHIFT11;
    }
    
    //pokud nebyl identifikován fázový posun, vrátí se 0.0
    if(cntr==0){
        return 0.0;
    }
    //pokud byl identifikován jen jeden posun - tento posun je řešení
    else if(cntr==1){
        return shift;
    }
    //jinak se vrátí výchozí hodnota - posun předchozího vzorku
    else{
        return previous_shift;
    }
}

/*
    Demoduluje synchronizační sekvenci.
    input vstupní soubor
    return délka symbolu
*/
int demodulate_sync(SndfileHandle& input){
    //buffer pro vzorek
    int buffer;
    
    //vzorkovací rychlost
    double sample_rate=input.samplerate();
    //čas jednoho vzorku
    double sample_time=1.0/sample_rate;
    
    //fázový posun vzorku
    double shift=0.0;
    //fázový posun předchozího vzorku
    double previous_shift=0.0;
    //počet přečtených vzorů
    int number_of_samples=0;
    
    //idenfikace posunů synchronizační sekvence, která má 4 symboly
    for(int n=0;n<4;n++){
        //identifikace jednoho symbolu, i je počitadlo vzorků symbolu
        for(int i=0;true;i++){
            //pokud došlo ke změně fázového posunu, tak byl nalezen nový symbol
            if(previous_shift!=shift&&i>0){
                break;
            }
            
            //načtení nového vzorku a zjištění fázového posunu
            previous_shift=shift;
            input.read(&buffer, 1);
            shift=get_shift(number_of_samples*sample_time,buffer,previous_shift);
            
            //při načtení prvního vzorku se provede úprava "předchozího" vzorku
            if(n==0&&i==0){
                previous_shift=shift;
            }
            number_of_samples++;
        }
    }
    
    //vypočtení počtu vzorků synchronizační sekvekvence
    number_of_samples=4*(number_of_samples-2)/3;
    
    //přečtení zbývajících vzorků synchronizační sekvekvence
    for(int i=0;i<number_of_samples/4-1;i++){
        input.read(&buffer, 1);
    }
    
    return number_of_samples/4;
}

/*
    Zapíše symbol do souboru.
    output výstupní soubor
    shift posun symbolu
*/
void write_shift(ofstream& output,double shift){
	if(shift==SHIFT00){
		output<<"00";
	}
	else if(shift==SHIFT01){
		output<<"01";
	}
	else if(shift==SHIFT10){
		output<<"10";
	}
	else if(shift==SHIFT11){
		output<<"11";
	}
	else{
		cerr<<"Cant write uknown shift."<<endl;
	}
}

/*
    Demoduluje vstupní soubor.
    input vstupní soubor
    output výstupní soubor
    symbol_length délka symbolu ve vzorcích
*/
void demodulate_file(SndfileHandle& input, ofstream& output, int symbol_length){
    //počet přečtených vzorků
    int cntr=symbol_length*4;
    //počet přečtených vzorků při posledním čtení
    int read=1;
    //počet přečtených vzorků současného symbolu
    int i=0;
    //buffer pro vzorku
    int buffer=0;
    //fázový posun symbolu
    double shift=0.0;
    //vzorkovací rychlost
    double sample_rate=input.samplerate();
    //čas jednoho vzorku
    double sample_time=1.0/sample_rate;
    
    for(;;){
    	shift=0.0;
    	i=0;
    	
    	//identifikace posunu symbolu
    	//pokud je posun identifikován cyklus se ukončí
    	do{
    	    //přečtení vzorku, pokud nebyl přečten ukončí se čtení symbolu
    		if((read=input.read(&buffer, 1))==0){
    			break;
    		}
    		cntr++;
    		i++;
    		shift=get_shift(cntr*sample_time,buffer,shift);
    	}
    	while(shift==0.0);
    	
    	//pokud nebyl přečten nový vzorek ukončí se demodulace
    	if(read==0){
    		break;
    	}
    	
    	//zapsání symbolu
    	write_shift(output,shift);
    	
    	//přeskočení zbývajících vzorků symbolu
    	for(;i<symbol_length;i++){
    		input.read(&buffer, 1);
    		cntr++;
    	}
    }
}

/*
    Hlavní funkce.
    argc počet argumentů příkazové řádky
    argv argumenty příkazové řádky
*/
int main(int argc, char** argv) {
    
    //kontrola počtu parametrů
    if(argc!=2){
        cerr<<"Wrong number of arguments."<<endl;
        return EXIT_FAILURE;
    }
    
    //kontrola jména vstupnního souboru
    string input_filename(argv[1]);
    if(input_filename.length()<5||(input_filename.substr(input_filename.length()-4,4)!=".wav")){
        cerr<<"Incorrect input filename."<<endl;
        return EXIT_FAILURE;
    }
    
    //otevření vstupního souboru
    SndfileHandle input;
    input=SndfileHandle(argv[1]);

    //délka symbolu ve vzorcích
    int symbol_length=demodulate_sync(input);
    
    //otevření výstupního souboru
    string output_filename=input_filename.substr(0,input_filename.length()-4)+".txt";
    ofstream output(output_filename);
    
    //demodulace
    demodulate_file(input,output,symbol_length);
    
    return EXIT_SUCCESS;
}

