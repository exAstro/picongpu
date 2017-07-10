/* Copyright 2014-2017 Rene Widera
 *
 *
 * This file is part of PIConGPU.
 *
 * PIConGPU is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PIConGPU is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PIConGPU.
 * If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

#include "simulation_defines.hpp"
#include "particles/filter/RelativeGlobalDomainPosition.def"

namespace picongpu
{
namespace particles
{
namespace filter
{

namespace acc
{
    template< typename T_Params >
    struct RelativeGlobalDomainPosition
    {
        using Params = T_Params;

        DINLINE RelativeGlobalDomainPosition(
            DataSpace< simDim > const & localDomainOffset,
            DataSpace< simDim > const & globalDomainSize,
            DataSpace< simDim > const & localSuperCellOffset
        ) :
            m_localDomainOffset( localDomainOffset ),
            m_globalDomainSize( globalDomainSize ),
            m_localSuperCellOffset( localSuperCellOffset )
        {
        }

        template< typename T_Particle >
        DINLINE bool operator()( T_Particle const & particle )
        {
            if( particle.isHandleValid( ) )
            {
                using SuperCellSize = typename T_Particle::SuperCellSize;
                /* offset of the superCell (in cells, without any guards) to the origin of the global domain */
                DataSpace< simDim > globalSuperCellOffset = m_localDomainOffset + (
                    m_localSuperCellOffset *
                    SuperCellSize::toRT( )
                );
                return isParticleInsideRange( particle, globalSuperCellOffset);
            }
            return false;
        }

    private:

        /** check if a particle is located in the user defined range
         *
         * @tparam T_Particle type of the particle
         * @param particle particle than needs to be checked
         * @param globalSuperCellOffset offset of the superCell (in cells, without any guards)
         *                              to the origin of the global domain
         */
        template< typename T_Particle >
        DINLINE bool isParticleInsideRange( T_Particle const & particle, DataSpace< simDim > const & globalSuperCellOffset ) const
        {
            using SuperCellSize = typename T_Particle::SuperCellSize;

            const int particleCellIdx = particle[ localCellIdx_ ];
            const DataSpace< simDim > cellInSuperCell( DataSpaceOperations< simDim >::
                template map< SuperCellSize >( particleCellIdx ) );
            const DataSpace< simDim > globalParticleOffset(
                globalSuperCellOffset +
                cellInSuperCell
            );

            const float_X relativePosition = float_X( globalParticleOffset[ Params::dimension ] ) /
                float_X( m_globalDomainSize[ Params::dimension ] );

            return ( Params::lowerBound <= relativePosition &&
                relativePosition < Params::upperBound );
        }

        DataSpace< simDim > m_localDomainOffset;
        DataSpace< simDim > m_globalDomainSize;
        DataSpace< simDim > m_localSuperCellOffset;
    };

} // namespace acc

    template< typename T_Params >
    struct RelativeGlobalDomainPosition
    {
        using Params = T_Params;

        template< typename T_SpeciesType >
        struct apply
        {
            using type = RelativeGlobalDomainPosition;
        };

        HINLINE RelativeGlobalDomainPosition( )
        {
            const SubGrid< simDim >& subGrid = Environment< simDim >::get( ).SubGrid( );
            globalDomainSize = subGrid.getGlobalDomain( ).size;
            localDomainOffset = subGrid.getLocalDomain( ).offset;
        }

        /** create filter for the accelerator
         *
         * @tparam T_WorkerCfg PMacc::mappings::threads::WorkerCfg, configuration of the worker
         * @param localSupercellOffset offset (in superCells, without any guards) relative
         *                        to the origin of the local domain
         * @param configuration of the worker
         */
        template< typename T_WokerCfg >
        DINLINE acc::RelativeGlobalDomainPosition< Params >
        operator( )(
            DataSpace< simDim > const & localSuperCellOffset,
            T_WokerCfg const &
        )
        {
            return acc::RelativeGlobalDomainPosition< Params >(
                localDomainOffset,
                globalDomainSize,
                localSuperCellOffset
            );

        }

        DataSpace< simDim > localDomainOffset;
        DataSpace< simDim > globalDomainSize;
    };

} //namespace filter
} //namespace particles
} //namespace picongpu
