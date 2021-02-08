
// Copyright 2020, Diogo Costa, diogo.pinhodacosta@canada.ca
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

#include "OpenWQ_chem.h"

/* #################################################
// Compute chemical transformations
################################################# */
void OpenWQ_chem::Run(
    OpenWQ_json& OpenWQ_json,
    OpenWQ_vars& OpenWQ_vars,
    OpenWQ_wqconfig& OpenWQ_wqconfig,
    OpenWQ_hostModelconfig& OpenWQ_hostModelconfig){
    
    // Local variable for num_HydroComp
   unsigned int num_hydrocmp = OpenWQ_hostModelconfig.HydroComp.size();

    // Loop over number of compartments
    for (unsigned int icmp=0;icmp<num_hydrocmp;icmp++){
    
        BGC_Transform( // calls exprtk: parsing expression
            OpenWQ_json,
            OpenWQ_vars,
            OpenWQ_hostModelconfig,
            OpenWQ_wqconfig,
            icmp); 

    }
}

/* #################################################
// Compute each chemical transformation
################################################# */
void OpenWQ_chem::BGC_Transform(
    OpenWQ_json& OpenWQ_json,
    OpenWQ_vars& OpenWQ_vars,
    OpenWQ_hostModelconfig& OpenWQ_hostModelconfig,
    OpenWQ_wqconfig& OpenWQ_wqconfig,
    unsigned int icmp){
    
    // Local variables for expression evaluator: exprtk
    std::string expression_string; // expression string
    typedef exprtk::symbol_table<double> symbol_table_t;
    typedef exprtk::expression<double> expression_t;
    typedef exprtk::parser<double> parser_t;

    // Other local variables
    std::string chemname; // chemical name
    unsigned int index_cons,index_prod; // indexed for consumed and produced chemical
    int index_i; // iteractive index (can be zero because it is determined in a .find())
    double param_val; // prameter value
    unsigned int num_BGCcycles; // number of BGC frameworks in comparment icmp
    unsigned int num_transf; // number of transformation in biogeochemical cycle bgci
    unsigned int nx = std::get<2>(OpenWQ_hostModelconfig.HydroComp.at(icmp)); // number of cell in x-direction
    unsigned int ny = std::get<3>(OpenWQ_hostModelconfig.HydroComp.at(icmp)); // number of cell in y-direction
    unsigned int nz = std::get<4>(OpenWQ_hostModelconfig.HydroComp.at(icmp)); // number of cell in z-direction
    std::string cyclingFrame_i; // BGC Cycling name i of compartment icmp 
    unsigned int index_express; // iteractive index for expression evaluator  

    // Find compartment icmp name from code (host hydrological model)
    std::string CompName_icmp = std::get<1>(
        OpenWQ_hostModelconfig.HydroComp.at(icmp)); // Compartment icomp name

    // Get cycling_frameworks for compartment icomp (name = CompName_icmp) 
    // (compartment names need to match)
    std::vector<std::string> BGCcycles_icmp = 
        OpenWQ_json.Config["BIOGEOCHEMISTRY_CONFIGURATION"]
            [CompName_icmp]["cycling_framework"];

    // Get number of BGC frameworks in comparment icmp
    num_BGCcycles = BGCcycles_icmp.size();
        

    /* ########################################
    // Loop over biogeochemical cycling frameworks
    ######################################## */

    for (unsigned int bgci=0;bgci<num_BGCcycles;bgci++){

        // Get framework name i of compartment icmp
        cyclingFrame_i = BGCcycles_icmp.at(bgci);

        // Get number transformations in biogeochemical cycle cyclingFrame_i
        num_transf = OpenWQ_json.BGCcycling["CYCLING_FRAMEWORKS"]
            [cyclingFrame_i]["list_transformations"].size();

        /* ########################################
        // Loop over transformations in biogeochemical cycle bgci
        ######################################## */
        
        for (unsigned int transi=0;transi<num_transf;transi++){

            std::vector<unsigned int> index_transf; // index of chemical in transformation equation (needs to be here for loop reset)

            // Get transformation transi info
            std::string consumed_spec =  OpenWQ_json.BGCcycling["CYCLING_FRAMEWORKS"][cyclingFrame_i]
                [std::to_string(transi+1)]["consumed"];
            std::string produced_spec =  OpenWQ_json.BGCcycling["CYCLING_FRAMEWORKS"][cyclingFrame_i]
                [std::to_string(transi+1)]["produced"];
            std::string expression_string = OpenWQ_json.BGCcycling["CYCLING_FRAMEWORKS"][cyclingFrame_i]
                [std::to_string(transi+1)]["kinetics"];
            std::vector<std::string> parameter_names = OpenWQ_json.BGCcycling["CYCLING_FRAMEWORKS"][cyclingFrame_i]
                [std::to_string(transi+1)]["parameter_names"];
            std::string expression_string_modif = expression_string;
            
            // Find species indexes: consumed, produced and in the expression
            
            for(unsigned int chemi=0;chemi<(*OpenWQ_wqconfig.num_chem);chemi++){
                
                // Get chemical species name
                chemname = (*OpenWQ_wqconfig.chem_species_list)[chemi];

                // Consumedchemass_consumed, chemass_produced;ty()) 
                index_i = consumed_spec.find(chemname);
                if (index_i!=-1 && !consumed_spec.empty()){
                    index_cons = chemi; // index
                }

                // Produced
                index_i = produced_spec.find(chemname);
                if (index_i!=-1 && !produced_spec.empty()){
                    index_prod = chemi; // index
                }

                // In expression
                index_i = expression_string.find(chemname);
                index_express = 0; // reset needed for every new equation
                if (index_i!=-1 && !expression_string.empty()){
                    index_transf.push_back(chemi); // index
                    expression_string_modif.replace(
                        index_i,
                        index_i + chemname.size(),
                        "chemass_InTransfEq["+std::to_string(index_express)+"]");
                    index_express++;
                }
            }

            // Replace parameter name by value in expression
            for (unsigned int i=0;i<parameter_names.size();i++){
                index_i = expression_string_modif.find(parameter_names[i]);
                param_val = OpenWQ_json.BGCcycling["CYCLING_FRAMEWORKS"][cyclingFrame_i]
                    [std::to_string(transi+1)]["parameter_values"][parameter_names[i]];
                expression_string_modif.replace(index_i,index_i + parameter_names[i].size(),std::to_string(param_val));
            }

            // Add variables to symbol_table
            symbol_table_t symbol_table;
            std::vector<double> chemass_InTransfEq; // chemical mass involved in transformation (needs to be here for loop reset)
            for (unsigned int i=0;i<index_transf.size();i++){
                chemass_InTransfEq.push_back(0); // creating the vector
            }
            symbol_table.add_vector("chemass_InTransfEq",chemass_InTransfEq);
            // symbol_table.add_constants();

            // Create Object
            expression_t expression;
            expression.register_symbol_table(symbol_table);

            // Parse expression and compile 
            parser_t parser;
            parser.compile(expression_string_modif,expression);

            /* ########################################
            // Loop over space: nx, ny, nz
            ######################################## */
            for (unsigned int ix=0;ix<nx;ix++){
                for (unsigned int iy=0;iy<ny;iy++){
                    for (unsigned int iz=0;iz<nz;iz++){
                        
                        // loop to get all the variables inside the expression
                        for (unsigned int chem=0;chem<chemass_InTransfEq.size();chem++){
                            chemass_InTransfEq[chem] = (*OpenWQ_vars.chemass)(icmp)(index_transf[chem])(ix,iy,iz);
                        }

                        // Mass transfered: Consumed -> Produced
                        double transf_mass = expression.value(); 

                        // New mass of consumed chemical
                        (*OpenWQ_vars.chemass)(icmp)(index_cons)(ix,iy,iz) -= transf_mass;

                        // New mass of produced chemical
                        (*OpenWQ_vars.chemass)(icmp)(index_prod)(ix,iy,iz) += transf_mass;

                    }
                }
            }
        }
    }

}