/**
 * @file This file is part of EDGE.
 *
 * @author Alexander Breuer (anbreuer AT ucsd.edu)
 *
 * @section LICENSE
 * Copyright (c) 2020, Friedrich Schiller University Jena
 * Copyright (c) 2019-2020, Alexander Breuer
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
 * Derives mesh partitions.
 **/
#ifndef EDGE_V_MESH_PARTITION_H
#define EDGE_V_MESH_PARTITION_H

#include "Mesh.h"

namespace edge_v {
  namespace mesh {
    class Partition;
  }
}

/**
 * Mesh-related functions and data.
 **/
class edge_v::mesh::Partition {
  private:
    //! mesh interface
    Mesh const & m_mesh;

    //! number of partitions
    t_idx m_nPas = 0;

    //! number of elements per partition
    t_idx *m_nPaEls = nullptr;

    //! partitions of the elements
    t_idx * m_elPa = nullptr;

    //! time groups of the elements
    unsigned short const * m_elTg = nullptr;

    //! priorities of the elements
    t_idx * m_elPr = nullptr;

    /**
     * Gets the number of elements per partition.
     *
     * @param i_nEls number of elements.
     * @param i_elPa partitions of the elements.
     * @param o_nPaEls will be set to the number of elements for the partitions.
     **/
    static void nPaEls( t_idx         i_nEls,
                        t_idx const * i_elPa,
                        t_idx       * o_nPaEls );

    /**
     * Gets the elements' priorities.
     * The priority is given as combination of the respective element's partition and time group.
     * Assume npa partitions and ntg time groups:
     *
     * * Inner elements of the first partition have priority based on their time group in [0, ntg-1].
     * * Send elements of the first parititon have priority based on their time group in [ntg, 2ntg-1].
     * * Inner elements of the second partition have priority based on their time group in [2ntg, 3ntg-1].
     * * [...]
     *
     * @param i_elTy element type.
     * @parma i_nEls number of elements.
     * @param i_elFaEl elements adjacent to elements (faces as bridge).
     * @param i_elPa partitions of the elements.
     * @param i_elTg time groups of the elements.
     * @param o_elPr will be set to elements' priorities.
     **/
    static void getElPr( edge_v::t_entityType         i_elTy,
                         t_idx                        i_nEls,
                         t_idx                const * i_elFaEl,
                         t_idx                const * i_elPa,
                         unsigned short       const * i_elTg,
                         t_idx                      * o_elPr );

  public:
    /**
     * Constructor.
     *
     * @param i_mesh mesh interface.
     * @param i_elTg time groups of the elements.
     **/
    Partition( Mesh           const & i_mesh,
               unsigned short const * i_elTg );

    /**
     * Destructor.
     **/
    ~Partition();

    /**
     * Uses Metis' PartGraphKway to determine the partitioning.
     *
     * @param i_nPars number of partitions to generate.
     * @param i_nCuts number of partitionings computed; the one with lowest comm volume is stored.
     **/
    void kWay( t_idx          i_nParts,
               unsigned short i_nCuts = 5 );

    /**
     * Gets the element to partition assignment.
     *
     * @return elPa info.
     **/
    t_idx const * getElPa() const { return m_elPa; }

    /**
     * Gets the elements' priorities (lower value = higher priority).
     *
     * @return elPr info.
     **/
    t_idx const * getElPr() const { return m_elPr; }

    /**
     * Gets the number of partitions.
     *
     * @return number of partitions.
     **/
    t_idx nPas() const { return m_nPas; }

    /**
     * Gets the number of elements per partition.
     *
     * @return number of elements for every partition.
     **/
    t_idx const * nPaEls() const { return m_nPaEls; }
};

#endif