/****************************************************************
*****************************************************************
    _/    _/  _/_/_/  _/       Numerical Simulation Laboratory
   _/_/  _/ _/       _/       Physics Department
  _/  _/_/    _/    _/       Universita' degli Studi di Milano
 _/    _/       _/ _/       Prof. D.E. Galli
_/    _/  _/_/_/  _/_/_/_/ email: Davide.Galli@unimi.it
*****************************************************************
*****************************************************************/

#include <iostream>
#include <fstream>
#include <ostream>
#include <cmath>
#include <iomanip>
#include "Monte_Carlo_ISING_1D.h"

using namespace std;

int main() {

  Clean();
  double sum=0.;
  
  do{ 
    Input(sum); //Inizialization
    cout << "TEMPERATURA" << temp << endl;
    for(int iblk=1; iblk <= nblk; ++iblk)
    {
      Reset(iblk);   //Reset block averages
      for(int istep=1; istep <= nstep; ++istep) //nstep è L, misure in un blocco.
      {
	Move(metro);
	Measure();
	Accumulate(); //Update block averages
      }
    Averages(iblk);   //Print results for current block
   }
	
   sum += 0.005;
} while (temp <= 3.0);

return 0;
}

void Clean(){
  remove("DatiM");
}

void Input(double sum)
{
  ifstream ReadInput;
/*
  cout << "Classic 1D Ising model             " << endl;
  cout << "Monte Carlo simulation             " << endl << endl;
  cout << "Nearest neighbour interaction      " << endl << endl;
  cout << "Boltzmann weight exp(- beta * H ), beta = 1/T " << endl << endl;
  cout << "The program uses k_B=1 and mu_B=1 units " << endl;
*/
//Read seed for random numbers
   int p1, p2;
   ifstream Primes("Primes");
   Primes >> p1 >> p2 ;
   Primes.close();

   ifstream input("seed.in");
   input >> seed[0] >> seed[1] >> seed[2] >> seed[3];
   rnd.SetRandom(seed,p1,p2);
   input.close();
  
//Read input informations
  ReadInput.open("input.dat");

  ReadInput >> temp;
  temp += sum; 
  beta = 1.0/temp;
//  cout << "Temperature = " << temp << endl;

  ReadInput >> nspin;
//  cout << "Number of spins = " << nspin << endl;

  ReadInput >> J;
//  cout << "Exchange interaction = " << J << endl;

  ReadInput >> h;
//  cout << "External field = " << h << endl << endl;
    
  ReadInput >> metro; // if=1 Metropolis else Gibbs

  ReadInput >> nblk;

  ReadInput >> nstep;

  ReadInput >> boolean;

  ReadInput >> cicli;
/*
  if(metro==1) cout << "The program perform Metropolis moves" << endl;
  else cout << "The program perform Gibbs moves" << endl;
  cout << "Number of blocks = " << nblk << endl;
  cout << "Number of steps in one block = " << nstep << endl << endl;
*/
  ReadInput.close();


//Prepare arrays for measurements
  iu = 0; //Energy
  ic = 1; //Heat capacity
  im = 2; //Magnetization
  ix = 3; //Magnetic susceptibility
 
  n_props = 4; //Number of observables

//initial configuration
  for (int i=0; i<nspin; ++i)
  {
    if(rnd.Rannyu() >= 0.5) s[i] = 1;
    else s[i] = -1;
  }
  
//Evaluate energy etc. of the initial configuration
  Measure();

//Print initial values for the potential energy and virial
 // cout << "Initial energy = " << walker[iu]/(double)nspin << endl;
}


void Move(int metro){
  int o;
  double diff;
  double p, energy_old, energy_new, sm;
  double energy_up, energy_down;
  double rapp;

   
  if(metro == 0){   //Gibbs
    for(int i=0; i<nspin; ++i){
    //Select randomly a particle 
      o = (int)(rnd.Rannyu()*nspin);
	
      energy_up=Boltzmann(1,o);
      energy_down=Boltzmann(-1,o);
      p=1./(1.0+exp(-beta*(energy_down-energy_up))); //probabilità di avere lo spin up indipendentemente dal resto.
 
      if( p >= rnd.Rannyu()){
        s[o]=1;
      } else {
        s[o]=-1;
      }
     }
  } else {
    for(int i=0; i<nspin; ++i){
      //Select randomly a particle 
      o = (int)(rnd.Rannyu()*nspin);
      attempted++; 
	
      energy_old=Boltzmann(s[o],o);
      energy_new=Boltzmann(-s[o],o); 
      diff=energy_new-energy_old;
      p=exp(-(double)diff*beta);
      if(diff < 0){//è energicamente conveniente girare lo spin.
        s[o]=-s[o];
	accepted++;
      } else {
	if(rnd.Rannyu() < p){
	  s[o]=-s[o];
	  accepted++;
	}
      }	
    }
  }
}

double Boltzmann(int sm, int ip){
  double ene = -J * sm * ( s[Pbc(ip-1)] + s[Pbc(ip+1)] ) - h * sm;
  return ene;
}

void Measure(){

  int bin;
  double u = 0.0, m = 0.0;

//cycle over spins
  for (int i=0; i<nspin; ++i){
     u += -J * s[i] * s[Pbc(i+1)] - 0.5 * h * (s[i] + s[Pbc(i+1)]);
     m += s[i];
  }

  walker[iu] = u;
  walker[ic] = pow(u,2);
  walker[im] = m;
  walker[ix] = pow(m,2);
}

void Reset(int iblk) //Reset block averages
{
   
   if(iblk == 1)
   {
       for(int i=0; i<n_props; ++i)
       {
           glob_av[i] = 0;
           glob_av2[i] = 0;
       }
   }

   for(int i=0; i<n_props; ++i)
   {
     blk_av[i] = 0;
   }
   blk_norm = 0;
   attempted = 0;
   accepted = 0;
}

void Accumulate(void) //Update block averages
{

   for(int i=0; i<n_props; ++i)
   {
     blk_av[i] = blk_av[i] + walker[i];
   }
   blk_norm = blk_norm + 1.0;
}

void Averages(int iblk) //Print results for current block
{

   ofstream DatiU, DatiCS, DatiM, DatiCHI; 
   const int wd=12;
    
    //cout << "Block number " << iblk << endl;
    //cout << "Acceptance rate " << accepted/attempted << endl << endl;

    DatiM.open("DatiM",ios::app);
    
    stima_m = blk_av[im]/blk_norm/(double)nspin; //Magnetizzazione per spin.
   
    //Magnetizzazione.
    glob_av[im]  += stima_m;
    glob_av2[im] += stima_m*stima_m;
    err_m=Error(glob_av[im],glob_av2[im],iblk);

    if(iblk == nblk){ //Salvo le misure dell'ultimo blocco.	
        DatiM << temp << " " << glob_av[im]/(double)iblk << " " << err_m << endl;
        cout << temp << " " << glob_av[im]/(double)iblk << " " << err_m <<  endl;
    }
 
    DatiM.close();
    
   // cout << "----------------------------" << endl << endl;
}


void ConfFinal(void)
{
  ofstream WriteConf;

  cout << "Print final configuration to file config.final " << endl << endl;
  WriteConf.open("config.final");
  for (int i=0; i<nspin; ++i)
  {
    WriteConf << s[i] << endl;
  }
  WriteConf.close();

  rnd.SaveSeed();
}

int Pbc(int i)  //Algorithm for periodic boundary conditions
{
    if(i >= nspin) i = i - nspin;
    else if(i < 0) i = i + nspin;
    return i;
}

double Error(double sum, double sum2, int iblk)
{
    return sqrt((sum2/(double)iblk - pow(sum/(double)iblk,2))/(double)iblk);
}

/****************************************************************
*****************************************************************
    _/    _/  _/_/_/  _/       Numerical Simulation Laboratory
   _/_/  _/ _/       _/       Physics Department
  _/  _/_/    _/    _/       Universita' degli Studi di Milano
 _/    _/       _/ _/       Prof. D.E. Galli
_/    _/  _/_/_/  _/_/_/_/ email: Davide.Galli@unimi.it
*****************************************************************
*****************************************************************/
