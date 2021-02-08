// Copyright 2020, Diogo Costa
// This file is part of OpenWQ model.

// This program, openWQ, is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) aNCOLS later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef GLOBALH_INCLUDED
#define GLOBALH_INCLUDED

#include <string>
#include <armadillo>
#include <memory> 


#include "jnlohmann/json.h"
using json = nlohmann::json;

/* #################################################
// Project JSON files
################################################# */
class OpenWQ_json
{
    public:

    json Master;
    json Config;
    json BGCcycling;
    json SinkSource;

};

/* #################################################
// Link: openWQ to Host Hydrological Model
################################################# */
class OpenWQ_hostModelconfig
{
    #include <tuple>
    #include <vector>

    typedef std::tuple<int,std::string,int, int, int> hydroTuple;
    // Add host_hydrological_model compartment:
    // (1) int => index in openWQ 
    // (2) std::string => reference name in JSON file
    // (3) int => number of cells in x-direction
    // (4) int => number of cells in y-direction
    // (5) int => number of cells in z-direction
    
    public: 
    
    std::vector<hydroTuple> HydroComp;
    
    // Number of hydrological compartments (that can store and transport water)
    unsigned int num_HydroComp;

};

/* #################################################
// General information for openWQ about the project
################################################# */
class OpenWQ_wqconfig
{

    public:

    // Chemical species
    std::unique_ptr
        <std::vector
        <std::string>> chem_species_list;    // list
    std::unique_ptr
        <unsigned int> num_chem;             // number of chemical species         

};

/* #################################################
// Main openWQ data structures
################################################# */
class OpenWQ_vars
{
    public:
    OpenWQ_vars(){

    }
    OpenWQ_vars(size_t num_HydroComp){

        this-> num_HydroComp = num_HydroComp;

        try{

            // Units of chemass are in: g (grams)
            // Thus, concentrations are in mg/l (or g/m3) and volume in m3
            chemass = std::unique_ptr<
                arma::field< // Compartments
                arma::field< // Chemical Species
                arma::Cube<  // Dimensions: nx, ny, nz
                double>>>>(new arma::field<arma::field<arma::cube>>(num_HydroComp)); 

        }catch(const std::exception& e){
            std::cout << 
                "ERROR: An exception occured during memory allocation (openWQ_global.h)" 
                << std::endl;
            exit (EXIT_FAILURE);
        }

    }
    size_t num_HydroComp;

    std::unique_ptr<arma::field<arma::field<arma::Cube<double>>>> chemass; 
    
};



#endif