/**
 * @file This file is part of EDGE.
 *
 * @author Alexander Breuer (anbreuer AT ucsd.edu)
 *
 * @section LICENSE
 * Copyright (c) 2019, Alexander Breuer
 * Copyright (c) 2018, Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 * Interface to MOAB.
 **/
#ifndef EDGE_V_IO_MOAB_H
#define EDGE_V_IO_MOAB_H

#include <moab/Interface.hpp>
#include <moab/Core.hpp>
#include <string>
#include "constants.h"
#include "logging.h"

namespace edge_v {
  namespace io {
    class Moab;
  }
}

/**
 * MOAB interface.
 **/
class edge_v::io::Moab {
  private:
    //! moab interface
    moab::Interface *m_moab;

    //! root set
    moab::EntityHandle m_root;

    //! number points
    std::size_t m_nPoint;

    //! number of line elements
    std::size_t m_nLine;

    //! number quad4r elements
    std::size_t m_nQuad4r;

    //! number of tria3 elements
    std::size_t m_nTria3;

    //! number of hex8r elements
    std::size_t m_nHex8r;

    //! number of tet4 elements
    std::size_t m_nTet4;

    /**
     * Converts the given type to a native MOAB type.
     *
     * @param i_enTy EDGE-V entity type.
     * @return corresponding MOAB type.
     **/
    static moab::EntityType getMoabType( t_entityType i_enTy );

    /**
     * Sets the given data in MOAB.
     *
     * @param i_enTy entity type to which this data belongs.
     * @param i_daTy used MOAB data type.
     * @param i_tagName tag name.
     * @param i_data data, which will be stored.
     *
     * @paramt TL_T type of the data, which is written.
     **/
    template< typename TL_T >
    void setEnData( t_entityType        i_enTy,
                    moab::DataType      i_daTy,
                    std::string const & i_tagName,
                    TL_T        const * i_data ) {
      moab::EntityType l_ty = getMoabType(  i_enTy );

      // get the entities by type
      std::vector< moab::EntityHandle > l_ens;
      moab::ErrorCode l_err = m_moab->get_entities_by_type( 0,
                                                            l_ty,
                                                            l_ens );
      EDGE_V_CHECK_EQ( l_err, moab::MB_SUCCESS );

      // create the tag
      moab::Tag l_tag;
      l_err = m_moab->tag_get_handle( i_tagName.c_str(),
                                      1,
                                      i_daTy,
                                      l_tag,
                                      moab::MB_TAG_CREAT|moab::MB_TAG_DENSE );
      EDGE_V_CHECK_EQ( l_err, moab::MB_SUCCESS );

      // store the data
      l_err = m_moab->tag_set_data( l_tag,
                                    &l_ens[0],
                                    l_ens.size(),
                                    i_data );
      EDGE_V_CHECK_EQ( l_err, moab::MB_SUCCESS );
    }

  public:
    /**
     * Constructs a new Moab object.
     * 
     * @param i_pathToMesh path to the mesh.
     **/
    Moab( std::string const & i_pathToMesh );

    /**
     * Destroys the Moab object.
     **/
    ~Moab();

    /**
     * Prints mesh statistics.
     **/
    void printStats() const;

    /**
     * Gets the entity type of the mesh's elements.
     *
     * @return element type of the mesh.
     **/
    t_entityType getElType() const;

    /**
     * Gets the number of entities by the number dimensions.
     * 
     * @param i_nDis number of dimensions.
     * @return number of entities.
     **/
    std::size_t nEnsByDis( unsigned short i_nDis ) const;

    /**
     * Gets the number of entities by entity type.
     * 
     * @param i_enTy entity type.
     * @return number of entities.
     **/
    std::size_t nEnsByType( t_entityType i_enTy ) const;

    /**
     * Gets the coordinates of the (ordered) vertices.
     *
     * @param o_veCrds will be set to coordinates of the vertices.
     **/
    void getVeCrds( double(*o_veCrds)[3] ) const;

    /**
     * Gets the connectivity information for the given entity type.
     * The vertices are returned according to the entities canonical ordering.
     *   See: Tim Tautges, Canonical Numbering Systems for Finite-Element Codes.  
     * 
     * @param i_enTy entity type.
     * @param o_enVe will be set vertex ids (starting at 0) adjacent to the entities.
     **/
    void getEnVe( t_entityType   i_enTy,
                  std::size_t  * o_enVe ) const;

    /**
     * Gets the entity-to-entity adjacency (faces as bridge).
     *
     * @param i_enTy entity type.
     * @param o_enFaEn will be set to adjacency information.
     **/
    void getEnFaEn( t_entityType   i_enTy,
                    std::size_t  * o_enFaEn ) const;

    /**
     * Stores global mesh data.
     *
     * @param i_tagName tag name.
     * @param i_nValues number of values.
     * @param i_data data, which will be stored.
     **/
    void setGlobalData( std::string const & i_tagName,
                        std::size_t         i_nValues,
                        double      const * i_data );

    /**
     * Gets the number of values for the global data item.
     *
     * @param i_tagName tag name.
     * @return size of the global data.
     **/
    std::size_t getGlobalDataSize( std::string const & i_tagName ) const;

    /**
     * Gets global mesh data.
     *
     * @param i_tagName tag name.
     * @param o_data output array.
     **/
    void getGlobalData( std::string const & i_tagName,
                        double            * o_data ) const;

    /**
     * Sets the given data in MOAB (stored as native double).
     *
     * @param i_enTy entity type to which this data belongs.
     * @param i_tagName tag name.
     * @param i_data data, which will be stored.
     **/
    void setEnData( t_entityType        i_enTy,
                    std::string const & i_tagName,
                    double      const * i_data );

    /**
     * Sets the given data in MOAB (stored as native int).
     *
     * @param i_enTy entity type to which this data belongs.
     * @param i_tagName tag name.
     * @param i_data data, which will be stored.
     **/
    void setEnData( t_entityType           i_enTy,
                    std::string    const & i_tagName,
                    unsigned short const * i_data );

    /**
     * Gets the data for the given entity type (stored as native int).
     *
     * @param i_enTy entity type to which this data belongs.
     * @param i_tagName tag name.
     * @param o_data output array.
     **/
    void getEnData( t_entityType           i_enTy,
                    std::string    const & i_tagName,
                    unsigned short       * o_data ) const;

    /**
     * Gets the names of the tags.
     *
     * @param o_tagNames will be set to the names of the tags.
     **/
    void getTagNames( std::vector< std::string > & o_tagNames ) const;

    /**
     * Deletes the given tag, if it exists.
     *
     * @param i_tagName name of the tag which will be deleted.
     * @return true if the tag was delete, false if not (didn't exist).
     **/
    bool deleteTag( std::string const & i_tagName );

    /**
     * Reorders the entities by the given int-tag (ascending).
     *
     * @param i_enTy type of the entities, which are reordered.
     * @param i_tagName name of the tag.
     * @param i_skipValue tag with this value are skipped in the reordering.
     **/
    void reorder( t_entityType        i_enTy,
                  std::string const & i_tagName,
                  int                 i_skipValue = std::numeric_limits< int >::max() );

    /**
     * Writes the MOAB database to the given file.
     *
     * @param i_pathToMesh path to the mesh.
     **/
    void writeMesh( std::string const & i_pathToMesh );
};

#endif