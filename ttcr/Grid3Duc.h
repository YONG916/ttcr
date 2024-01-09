//
//  Grid3Duc.h
//  ttcr
//
//  Created by Bernard Giroux on 2014-02-24.
//  Copyright (c) 2014 Bernard Giroux. All rights reserved.
//  Copyright (c) 2018 Bernard Giroux, Maher Nasr. All rights reserved.
//

/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef ttcr_Grid3Duc_h
#define ttcr_Grid3Duc_h

#include <cassert>
#include <cmath>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#ifdef VTK
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-deprecated-sync"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProbeFilter.h"
#include "vtkRectilinearGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkSmartPointer.h"
#include "vtkTetra.h"
#include "vtkXMLRectilinearGridWriter.h"
#include "vtkXMLUnstructuredGridWriter.h"
#pragma clang diagnostic pop
#endif

#include "Grad.h"
#include "Grid3D.h"
#include "utils.h"

namespace ttcr {

    template<typename T1, typename T2, typename NODE>
    class Grid3Duc : public Grid3D<T1,T2> {
    public:
        Grid3Duc(const std::vector<sxyz<T1>>& no,
                 const std::vector<tetrahedronElem<T2>>& tet,
                 const int rp, const bool ttrp, const T1 md,
                 const size_t nt=1,
                 const bool _translateOrigin=false) :
        Grid3D<T1,T2>(ttrp, tet.size(), nt, _translateOrigin),
        rp_method(rp),
        nPrimary(static_cast<T2>(no.size())),
        source_radius(0.0), min_dist(md),
        nodes(std::vector<NODE>(no.size(), NODE(nt))),
        slowness(std::vector<T1>(tet.size())),
        tetrahedra(tet)
        {}

        virtual ~Grid3Duc() {}

        void setSlowness(const T1 s) {
            for ( size_t n=0; n<slowness.size(); ++n ) {
                slowness[n] = s;
            }
        }

        void setSlowness(const T1 *s, const size_t ns) {
            if ( slowness.size() != ns ) {
                throw std::length_error("Error: slowness vectors of incompatible size.");
            }
            for ( size_t n=0; n<slowness.size(); ++n ) {
                slowness[n] = s[n];
            }
        }

        void setSlowness(const std::vector<T1>& s) {
            if ( slowness.size() != s.size() ) {
                throw std::length_error("Error: slowness vectors of incompatible size.");
            }
            for ( size_t n=0; n<slowness.size(); ++n ) {
                slowness[n] = s[n];
            }
        }

        void getSlowness(std::vector<T1>& s) const {
            if (s.size() != slowness.size()) {
                s.resize(slowness.size());
            }
            for (size_t n=0; n<s.size(); ++n) {
                s[n] = slowness[n];
            }
        }

        void setSourceRadius(const double r) { source_radius = r; }

        void setTT(const T1 tt, const size_t nn, const size_t nt=0) {
            nodes[nn].setTT(tt, nt);
        }

        size_t getNumberOfNodes() const { return nodes.size(); }

        void getTT(std::vector<T1>& tt, const size_t threadNo=0) const final {
            tt.resize(nPrimary);
            for ( size_t n=0; n<nPrimary; ++n ) {
                tt[n] = nodes[n].getTT(threadNo);
            }
        }

        const T1 getXmin() const {
            T1 xmin = nodes[0].getX();
            for ( auto it=nodes.begin(); it!=nodes.end(); ++it ) {
                xmin = xmin<it->getX() ? xmin : it->getX();
            }
            return xmin;
        }
        const T1 getXmax() const {
            T1 xmax = nodes[0].getX();
            for ( auto it=nodes.begin(); it!=nodes.end(); ++it ) {
                xmax = xmax>it->getX() ? xmax : it->getX();
            }
            return xmax;
        }
        const T1 getYmin() const {
            T1 ymin = nodes[0].getY();
            for ( auto it=nodes.begin(); it!=nodes.end(); ++it ) {
                ymin = ymin<it->getY() ? ymin : it->getY();
            }
            return ymin;
        }
        const T1 getYmax() const {
            T1 ymax = nodes[0].getY();
            for ( auto it=nodes.begin(); it!=nodes.end(); ++it ) {
                ymax = ymax>it->getY() ? ymax : it->getY();
            }
            return ymax;
        }
        const T1 getZmin() const {
            T1 zmin = nodes[0].getZ();
            for ( auto it=nodes.begin(); it!=nodes.end(); ++it ) {
                zmin = zmin<it->getZ() ? zmin : it->getZ();
            }
            return zmin;
        }
        const T1 getZmax() const {
            T1 zmax = nodes[0].getZ();
            for ( auto it=nodes.begin(); it!=nodes.end(); ++it ) {
                zmax = zmax>it->getZ() ? zmax : it->getZ();
            }
            return zmax;
        }

        void getRaypath(const std::vector<sxyz<T1>>& Tx,
                        const std::vector<T1>& t0,
                        const sxyz<T1> &Rx,
                        std::vector<sxyz<T1>> &r_data,
                        T1 &tt,
                        const size_t threadNo) const;

        void getRaypath(const std::vector<sxyz<T1>>& Tx,
                        const std::vector<T1>& t0,
                        const sxyz<T1> &Rx,
                        std::vector<siv<T1>> &l_data,
                        T1 &tt,
                        const size_t threadNo) const;

        void saveTT(const std::string &, const int, const size_t nt=0,
                    const int format=1) const;

#ifdef VTK
        void saveModelVTU(const std::string &, const bool saveSlowness=true,
                          const bool savePhysicalEntity=false) const;
        void saveModelVTR(const std::string &, const double*,
                          const bool saveSlowness=true) const;
#endif
        void saveModelCRT(const std::string &, const double*) const;

        void saveModelXYZ(const std::string &) const;

        void dump_secondary(std::ofstream& os) const {
            for ( size_t n=nPrimary; n<nodes.size(); ++n ) {
                os << nodes[n].getX() << ' ' << nodes[n].getY() << ' ' << nodes[n].getZ() << '\n';
            }
        }

        const T1 getAverageEdgeLength() const;

    protected:
        int rp_method;
        T2 nPrimary;
        T1 source_radius;
        T1 min_dist;
        mutable std::vector<NODE> nodes;
        std::vector<T1> slowness;
        std::vector<tetrahedronElem<T2>> tetrahedra;

        T1 computeDt(const NODE& source, const sxyz<T1>& node,
                     const size_t cellNo) const {
            return slowness[cellNo] * source.getDistance( node );
        }

        T1 computeDt(const NODE& source, const NODE& node,
                     const size_t cellNo) const {
            return slowness[cellNo] * source.getDistance( node );
        }

        T1 getTraveltime(const sxyz<T1>& Rx,
                         const size_t threadNo) const;

        T1 getTraveltime(const sxyz<T1>& Rx,
                         const std::vector<NODE>& nodes,
                         const size_t threadNo) const;

        T1 getTraveltime(const sxyz<T1>& Rx,
                         const std::vector<NODE>& nodes,
                         T2& nodeParentRx,
                         T2& cellParentRx,
                         const size_t threadNo) const;

        void checkPts(std::vector<sxyz<T1>> pts, const bool translated=false) const;

        bool insideTetrahedron(const sxyz<T1>&, const T2) const;
        bool insideTetrahedron2(const sxyz<T1>&, const T2) const;

        T2 getCellNo(const sxyz<T1>& pt) const;

        void buildGridNodes(const std::vector<sxyz<T1>>&, const size_t);
        void buildGridNodes(const std::vector<sxyz<T1>>&,
                            const T2, const size_t);

        void localUpdate3D(NODE *vertexC, const size_t threadNo) const;

        T1 localUpdate2D(const NODE *vertexA,
                         const NODE *vertexB,
                         const NODE *vertexC,
                         const T2 tetraNo,
                         const size_t threadNo) const;

        void local3Dsolver(NODE *vertexC, const size_t threadNo) const;

        T1 local2Dsolver(const NODE *vertexA,
                         const NODE *vertexB,
                         const NODE *vertexC,
                         const T2 tetraNo,
                         const size_t threadNo) const;

        int solveEq23(const T1 a[], const T1 b[], T1 n[][3]) const;

        T1 getTraveltimeFromRaypath(const std::vector<sxyz<T1>>& Tx,
                                    const std::vector<T1>& t0,
                                    const sxyz<T1> &Rx,
                                    const size_t threadNo) const;

        void getRaypath(const std::vector<sxyz<T1>>& Tx,
                        const sxyz<T1> &Rx,
                        std::vector<sxyz<T1>> &r_data,
                        const size_t threadNo) const;

        void getStraightRays(const std::vector<sxyz<T1>>& Tx,
                             const std::vector<sxyz<T1>>& Rx,
                             std::vector<std::vector<siv<T1>>>& l_data) const;

        void getStraightRays(const std::vector<sxyz<T1>>& Tx,
                             const std::vector<sxyz<T1>>& Rx,
                             std::vector<std::vector<sxyz<T1>>>& r_data) const;

        bool check_pt_location(sxyz<T1> &curr_pt,
                               const std::array<T2,3> &ind,
                               bool &onNode,
                               T2 &nodeNo,
                               bool &onEdge,
                               std::array<T2,2> &edgeNodes,
                               bool &onFace,
                               std::array<T2,3> &faceNodes) const;

        bool check_pt_location(sxyz<T1> &curr_pt,
                               const std::vector<T2> &ind1,
                               const std::array<T2,3> &ind2,
                               bool &onNode,
                               T2 &nodeNo,
                               bool &onEdge,
                               std::array<T2,2> &edgeNodes,
                               bool &onFace,
                               std::array<T2,3> &faceNodes) const;

        bool intersectVecTriangle(const T2 iO, const sxyz<T1> &vec,
                                  const T2 iA, T2 iB, T2 iC,
                                  sxyz<T1> &pt_i) const;
        bool intersectVecTriangle(const sxyz<T1> &O, const sxyz<T1> &vec,
                                  const T2 iA, T2 iB, T2 iC,
                                  sxyz<T1> &pt_i) const;

        bool intersectVecEdge(const sxyz<T1>& curr_pt,
                              const sxyz<T1>& g,
                              std::array<T2,3>& faceNodes,
                              sxyz<T1>&  pt_i,
                              std::array<T2,2>& edgeNodes) const;

        T2 findAdjacentCell1(const std::array<T2,3> &faceNodes, const T2 nodeNo) const;
        T2 findAdjacentCell2(const std::array<T2,3> &faceNodes, const T2 cellNo) const;

        void getNeighborNodes(const T2, std::set<NODE*>&) const;
        void getNeighborNodesAB(const std::vector<NODE*>&,
                                std::vector<std::vector<std::array<NODE*,3>>>&) const;

        void plotCell(const T2 cellNo, const sxyz<T1> &pt, const sxyz<T1> &g) const;

        void printRaypathData(const sxyz<T1>& curr_pt,
                              const sxyz<T1>& g,
                              const bool onNode,
                              const bool onEdge,
                              const bool onFace,
                              const T2 cellNo,
                              const T2 nodeNo,
                              const std::array<T2,2> &edgeNodes,
                              const std::array<T2,3> &faceNodes) const;

        std::array<T2,4> getPrimary(const T2 cellNo) const {
            size_t i = 0;
            std::array<T2,4> tmp;
            for (size_t n=0; n<this->neighbors[cellNo].size(); ++n) {
                if ( nodes[this->neighbors[cellNo][n]].isPrimary() )
                    tmp[i++] = this->neighbors[cellNo][n];
                if ( i == 4 ) break;
            }
            return tmp;
        }

        void checkCloseToTx(const sxyz<T1>& curr_pt,
                            sxyz<T1>& g,
                            const T2 cellNo,
                            const std::vector<sxyz<T1>>& Tx,
                            const std::vector<T2>& txCell) const {

            for (size_t nt=0; nt<Tx.size(); ++nt) {
                std::array<T2,4> itmp = getPrimary(txCell[nt]);
                for (size_t n=0; n<4; ++n){
                    for (auto nc=nodes[itmp[n]].getOwners().begin(); nc!=nodes[itmp[n]].getOwners().end(); ++nc){
                        if ( *nc==cellNo ) {
                            g = Tx[nt]-curr_pt;
                            return;
                        }
                    }
                }
            }
        }

        void checkCloseToTx(const sxyz<T1>& curr_pt,
                            sxyz<T1>& g,
                            const std::array<T2,2> &edgeNodes,
                            const std::vector<sxyz<T1>>& Tx,
                            const std::vector<T2>& txCell) const {

            for (size_t nt=0; nt<Tx.size(); ++nt) {
                std::array<T2,4> itmp = getPrimary(txCell[nt]);
                for (size_t n=0; n<4; ++n){
                    if ( itmp[n]==edgeNodes[0] || itmp[n]==edgeNodes[1] ) {
                        g = Tx[nt]-curr_pt;
                        return;
                    }
                }
            }
        }

        sxyz<T1> projectOnFace(const sxyz<T1>& g, const std::array<T2,3>& faceNodes) const {

            // calculate normal to the face
            sxyz<T1> v1 = {nodes[faceNodes[0]].getX()-nodes[faceNodes[1]].getX(),
                nodes[faceNodes[0]].getY()-nodes[faceNodes[1]].getY(),
                nodes[faceNodes[0]].getZ()-nodes[faceNodes[1]].getZ()};
            sxyz<T1> v2 = {nodes[faceNodes[0]].getX()-nodes[faceNodes[2]].getX(),
                nodes[faceNodes[0]].getY()-nodes[faceNodes[2]].getY(),
                nodes[faceNodes[0]].getZ()-nodes[faceNodes[2]].getZ()};
            sxyz<T1> n = cross(v1, v2);
            n.normalize();
            return cross(n, cross(g, n));
        }

        sxyz<T1> projectOnFace(const sxyz<T1>& curr_pt,
                               const sxyz<T1>& g,
                               std::array<T2,2>& edgeNodes,
                               const std::vector<T2> cells,
                               sxyz<T1>&  pt_i) const;

        bool projectOnFace(const sxyz<T1>& curr_pt,
                           const T2 nodeNo,
                           sxyz<T1>& g,
                           std::array<T2,2>& edgeNodes,
                           sxyz<T1>& pt_i) const;
    };

    template<typename T1, typename T2, typename NODE>
    T2 Grid3Duc<T1,T2,NODE>::getCellNo(const sxyz<T1>& pt) const {
        T2 closestNode = 0;
        T1 distance = std::numeric_limits<T1>::max();
        for (auto node=nodes.begin(); node!=nodes.begin()+nPrimary; ++node){
            T1 dist = pt.getDistance(*node);
            if (dist < distance) {
                distance = dist;
                closestNode = node->getGridIndex();
            }
        }
        T1 minVolumeDiff = std::numeric_limits<T1>::max();
        T2 cell = std::numeric_limits<T2>::max();

        for (auto tet=nodes[closestNode].getOwners().begin(); tet!=nodes[closestNode].getOwners().end(); ++tet) {
            T2 celli = *tet;
            for (size_t n=0;n<4;++n){
                T2 neighborNode = this->neighbors[celli][n];
                for (auto tet2=nodes[neighborNode].getOwners().begin(); tet2!=nodes[neighborNode].getOwners().end(); ++tet2) {
                    sxyz<T1> v1 = { nodes[ this->neighbors[*tet2][0] ]};
                    sxyz<T1> v2 = { nodes[ this->neighbors[*tet2][1] ]};
                    sxyz<T1> v3 = { nodes[ this->neighbors[*tet2][2] ]};
                    sxyz<T1> v4 = { nodes[ this->neighbors[*tet2][3] ]};

                    T1 D0 = 1.e6*det4(v1, v2, v3, v4);
                    T1 D1 = 1.e6*det4(pt, v2, v3, v4);
                    T1 D2 = 1.e6*det4(v1, pt, v3, v4);
                    T1 D3 = 1.e6*det4(v1, v2, pt, v4);
                    T1 D4 = 1.e6*det4(v1, v2, v3, pt);

                    T1 VolumeDiff = std::abs(std::abs(D0)-std::abs(D1)-std::abs(D2)-std::abs(D3)-std::abs(D4));
                    if (VolumeDiff < minVolumeDiff) {
                        minVolumeDiff = VolumeDiff;
                        cell = *tet2;
                    }
                }
            }
        }
        if ( cell >= tetrahedra.size() ) {
            throw std::runtime_error("Node cannot be found in mesh");
        }
        return cell;
    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::buildGridNodes(const std::vector<sxyz<T1>>& no,
                                              const size_t nt) {

        if ( this->translateOrigin ) {
            T1 xmin = no[0].x;
            T1 ymin = no[0].y;
            T1 zmin = no[0].z;
            for ( T2 n=1; n<no.size(); ++n ) {
                xmin = xmin < no[n].x ? xmin : no[n].x;
                ymin = ymin < no[n].y ? ymin : no[n].y;
                zmin = zmin < no[n].z ? zmin : no[n].z;
            }
            this->origin = {xmin, ymin, zmin};
        } else {
            this->origin = {0.0, 0.0, 0.0};
        }

        for ( T2 n=0; n<no.size(); ++n ) {
            nodes[n].setXYZindex(no[n].x - this->origin.x,
                                 no[n].y - this->origin.y,
                                 no[n].z - this->origin.z, n );
        }

        //
        //              1
        //            ,/|`\
        //          ,/  |  `\
        //        ,0    '.   `4
        //      ,/       1     `\
        //    ,/         |       `\
        //   0-----5-----'.--------3
        //    `\.         |      ,/
        //       `\.      |     3
        //          `2.   '. ,/
        //             `\. |/
        //                `2
        //
        //
        //  triangle 0:  0-1  1-2  2-0     (first occurence of segment underlined)
        //               ---  ---  ---
        //  triangle 1:  1-2  2-3  3-1
        //                    ---  ---
        //  triangle 2:  0-2  2-3  3-0
        //                         ---
        //  triangle 3:  0-1  1-3  3-0


        for ( T2 ntet=0; ntet<tetrahedra.size(); ++ntet ) {

            // for each triangle
            for ( T2 ntri=0; ntri<4; ++ntri ) {

                // push owner for primary nodes
                nodes[ tetrahedra[ntet].i[ntri] ].pushOwner( ntet );
            }
        }
    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::buildGridNodes(const std::vector<sxyz<T1>>& no,
                                              const T2 nsecondary,
                                              const size_t nt) {

        if ( this->translateOrigin ) {
            T1 xmin = no[0].x;
            T1 ymin = no[0].y;
            T1 zmin = no[0].z;
            for ( T2 n=1; n<no.size(); ++n ) {
                xmin = xmin < no[n].x ? xmin : no[n].x;
                ymin = ymin < no[n].y ? ymin : no[n].y;
                zmin = zmin < no[n].z ? zmin : no[n].z;
            }
            this->origin = {xmin, ymin, zmin};
        } else {
            this->origin = {0.0, 0.0, 0.0};
        }

        // primary nodes
        for ( T2 n=0; n<no.size(); ++n ) {
            nodes[n].setXYZindex( no[n].x, no[n].y, no[n].z, n );
            nodes[n].setPrimary(true);
        }
        T2 nNodes = static_cast<T2>(nodes.size());

        size_t nFaceNodes = 0;
        for ( size_t n=1; n<=(nsecondary-1); ++n ) {
            nFaceNodes += n;
        }

        std::map<std::array<T2,2>,std::vector<T2>> lineMap;
        std::array<T2,2> lineKey;
        typename std::map<std::array<T2,2>,std::vector<T2>>::iterator lineIt;

        size_t estLineNo = (tetrahedra.size()+tetrahedra.size()/10) * 6/2;
        size_t estFaceNo = (tetrahedra.size()+tetrahedra.size()/10) * 4/2;
        nodes.reserve( nNodes + estLineNo*nsecondary + estFaceNo*nFaceNodes );


        T2 iNodes[4][3] = {
            {0,1,2},  // (relative) indices of nodes of 1st triangle
            {1,2,3},  // (relative) indices of nodes of 2nd triangle
            {0,2,3},  // (relative) indices of nodes of 3rd triangle
            {0,1,3}   // (relative) indices of nodes of 4th triangle
        };

        //
        //              1
        //            ,/|`\
        //          ,/  |  `\
        //        ,0    '.   `4
        //      ,/       1     `\
        //    ,/         |       `\
        //   0-----5-----'.--------3
        //    `\.         |      ,/
        //       `\.      |     3
        //          `2.   '. ,/
        //             `\. |/
        //                `2
        //
        //
        //  triangle 0:  0-1  1-2  2-0     (first occurence of segment underlined)
        //               ---  ---  ---
        //  triangle 1:  1-2  2-3  3-1
        //                    ---  ---
        //  triangle 2:  0-2  2-3  3-0
        //                         ---
        //  triangle 3:  0-1  1-3  3-0

        if ( verbose>1 && nsecondary > 0 ) {
            std::cout << '\n';
        }

        // edge nodes
        NODE tmpNode(nt);
        for ( T2 ntet=0; ntet<tetrahedra.size(); ++ntet ) {

            if ( verbose>1 && nsecondary > 0 ) {
                std::cout << "\r  Building edge nodes: " << (100*ntet)/tetrahedra.size() << "%";
                std::cout.flush();
            }

            // for each triangle
            for ( T2 ntri=0; ntri<4; ++ntri ) {

                // push owner for primary nodes
                nodes[ tetrahedra[ntet].i[ntri] ].pushOwner( ntet );

                if ( nsecondary > 0 ) {
                    // start from ntri to avoid redundancy
                    for ( size_t nl=ntri; nl<3; ++nl ) {

                        lineKey = {tetrahedra[ntet].i[ iNodes[ntri][nl] ],
                            tetrahedra[ntet].i[ iNodes[ntri][(nl+1)%3] ]};
                        std::sort(lineKey.begin(), lineKey.end());

                        lineIt = lineMap.find( lineKey );
                        if ( lineIt == lineMap.end() ) {
                            // not found, insert new pair
                            lineMap[ lineKey ] = std::vector<T2>(nsecondary);
                        } else {
                            for ( size_t n=0; n<lineIt->second.size(); ++n ) {
                                // setting owners
                                nodes[ lineIt->second[n] ].pushOwner( ntet );
                            }
                            continue;
                        }

                        sxyz<T1> d = (no[lineKey[1]]-no[lineKey[0]])/static_cast<T1>(nsecondary+1);

                        for ( size_t n2=0; n2<nsecondary; ++n2 ) {
                            tmpNode.setXYZindex(no[lineKey[0]].x+(1+n2)*d.x,
                                                no[lineKey[0]].y+(1+n2)*d.y,
                                                no[lineKey[0]].z+(1+n2)*d.z,
                                                nNodes );
                            lineMap[lineKey][n2] = nNodes++;
                            nodes.push_back( tmpNode );
                            nodes.back().pushOwner( ntet );
                            nodes.back().setPrimary(false);
                        }
                    }
                }
            }
        }

        if ( verbose>1 && nsecondary > 0 ) {
            std::cout << "\r  Building edge nodes: 100%\n";
        }

        if ( nsecondary > 1 ) {

            std::map<std::array<T2,3>,std::vector<T2>> faceMap;
            std::array<T2,3> faceKey;
            typename std::map<std::array<T2,3>,std::vector<T2>>::iterator faceIt;

            int ncut = nsecondary - 1;

            for ( T2 ntet=0; ntet<tetrahedra.size(); ++ntet ) {

                if ( verbose>1 ) {
                    std::cout << "\r  Building face nodes: " << (100*ntet)/tetrahedra.size() << "%";
                    std::cout.flush();
                }

                // for each triangle
                for ( T2 ntri=0; ntri<4; ++ntri ) {

                    faceKey = {tetrahedra[ntet].i[ iNodes[ntri][0] ],
                        tetrahedra[ntet].i[ iNodes[ntri][1] ],
                        tetrahedra[ntet].i[ iNodes[ntri][2] ]};
                    std::sort(faceKey.begin(), faceKey.end());


                    faceIt = faceMap.find( faceKey );
                    if ( faceIt == faceMap.end() ) {
                        // not found, insert new pair
                        faceMap[ faceKey ] = std::vector<T2>(nFaceNodes);
                    } else {
                        for ( size_t n=0; n<faceIt->second.size(); ++n ) {
                            // setting owners
                            nodes[ faceIt->second[n] ].pushOwner( ntet );
                        }
                        continue;
                    }

                    sxyz<T1> d1 = (no[faceKey[1]]-no[faceKey[0]])/static_cast<T1>(nsecondary+1);
                    sxyz<T1> d2 = (no[faceKey[1]]-no[faceKey[2]])/static_cast<T1>(nsecondary+1);

                    size_t ifn = 0;
                    for ( ptrdiff_t n=0; n<ncut; ++n ) {

                        sxyz<T1> pt1 = no[faceKey[0]]+static_cast<T1>(1+n)*d1;
                        sxyz<T1> pt2 = no[faceKey[2]]+static_cast<T1>(1+n)*d2;

                        size_t nseg = ncut+1-n;

                        sxyz<T1> d = (pt2-pt1)/static_cast<T1>(nseg);

                        for ( size_t n2=0; n2<nseg-1; ++n2 ) {
                            tmpNode.setXYZindex(pt1.x+(1+n2)*d.x,
                                                pt1.y+(1+n2)*d.y,
                                                pt1.z+(1+n2)*d.z,
                                                nNodes );
                            faceMap[faceKey][ifn++] = nNodes++;
                            nodes.push_back( tmpNode );
                            nodes.back().pushOwner( ntet );
                            nodes.back().setPrimary(false);
                        }
                    }
                }
            }
        }
        if ( verbose>1 && nsecondary > 0 ) {
            std::cout << "\r  Building face nodes: 100%\n";
        }

        nodes.shrink_to_fit();

        if ( this->translateOrigin ) {
            for (auto node=nodes.begin(); node!=nodes.end(); ++node) {
                *node -= this->origin;
            }
        }
    }

    template<typename T1, typename T2, typename NODE>
    T1 Grid3Duc<T1,T2,NODE>::getTraveltime(const sxyz<T1>& Rx,
                                           const size_t threadNo) const {

        for ( size_t nn=0; nn<nodes.size(); ++nn ) {
            if ( nodes[nn] == Rx ) {
                return nodes[nn].getTT(threadNo);
            }
        }

        T2 cellNo = getCellNo( Rx );

        T2 neibNo = this->neighbors[cellNo][0];
        T1 dt = computeDt(nodes[neibNo], Rx, cellNo);

        T1 traveltime = nodes[neibNo].getTT(threadNo)+dt;
        for ( size_t k=1; k< this->neighbors[cellNo].size(); ++k ) {
            neibNo = this->neighbors[cellNo][k];
            dt = computeDt(nodes[neibNo], Rx, cellNo);
            if ( traveltime > nodes[neibNo].getTT(threadNo)+dt ) {
                traveltime =  nodes[neibNo].getTT(threadNo)+dt;
            }
        }
        return traveltime;
    }

    template<typename T1, typename T2, typename NODE>
    T1 Grid3Duc<T1,T2,NODE>::getTraveltime(const sxyz<T1>& Rx,
                                           const std::vector<NODE>& nodes,
                                           const size_t threadNo) const {

        for ( size_t nn=0; nn<nodes.size(); ++nn ) {
            if ( nodes[nn] == Rx ) {
                return nodes[nn].getTT(threadNo);
            }
        }

        T2 cellNo = getCellNo( Rx );

        T2 neibNo = this->neighbors[cellNo][0];
        T1 dt = computeDt(nodes[neibNo], Rx, cellNo);

        T1 traveltime = nodes[neibNo].getTT(threadNo)+dt;
        for ( size_t k=1; k< this->neighbors[cellNo].size(); ++k ) {
            neibNo = this->neighbors[cellNo][k];
            dt = computeDt(nodes[neibNo], Rx, cellNo);
            if ( traveltime > nodes[neibNo].getTT(threadNo)+dt ) {
                traveltime =  nodes[neibNo].getTT(threadNo)+dt;
            }
        }
        return traveltime;
    }

    template<typename T1, typename T2, typename NODE>
    T1 Grid3Duc<T1,T2,NODE>::getTraveltime(const sxyz<T1>& Rx,
                                           const std::vector<NODE>& nodes,
                                           T2& nodeParentRx, T2& cellParentRx,
                                           const size_t threadNo) const {

        for ( size_t nn=0; nn<nodes.size(); ++nn ) {
            if ( nodes[nn] == Rx ) {
                nodeParentRx = nodes[nn].getNodeParent(threadNo);
                cellParentRx = nodes[nn].getCellParent(threadNo);
                return nodes[nn].getTT(threadNo);
            }
        }

        T2 cellNo = getCellNo( Rx );
        T2 neibNo = this->neighbors[cellNo][0];
        T1 dt = computeDt(nodes[neibNo], Rx, cellNo);

        T1 traveltime = nodes[neibNo].getTT(threadNo)+dt;
        nodeParentRx = neibNo;
        cellParentRx = cellNo;
        for ( size_t k=1; k< this->neighbors[cellNo].size(); ++k ) {
            neibNo = this->neighbors[cellNo][k];
            dt = computeDt(nodes[neibNo], Rx, cellNo);
            if ( traveltime > nodes[neibNo].getTT(threadNo)+dt ) {
                traveltime = nodes[neibNo].getTT(threadNo)+dt;
                nodeParentRx = neibNo;
            }
        }
        return traveltime;
    }



    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::checkPts(std::vector<sxyz<T1>> pts, const bool translated) const {

        if (this->translateOrigin == true && translated == false) {
            for ( size_t n=0; n<pts.size(); ++n ) {
                pts[n] -= this->origin;
            }
        }

        for (size_t n=0; n<pts.size(); ++n) {
            bool found = false;
            // check first if point is on a node
            for ( T2 nt=0; nt<nodes.size(); ++nt ) {
                if ( nodes[nt] == pts[n]) {
                    found = true;
                    break;
                }
            }
            if ( found == false ) {
                // check if inside tetrahedra
                for ( T2 nt=0; nt<tetrahedra.size(); ++nt ) {
                    if ( insideTetrahedron(pts[n], nt) ) {
                        found = true;
                        break;
                    }
                }
            }
            if ( found == false ) {
                std::ostringstream msg;
                msg << "Error: Point (" << pts[n] << ") outside grid.";
                throw std::runtime_error(msg.str());
            }
        }
    }

    template<typename T1, typename T2, typename NODE>
    bool Grid3Duc<T1,T2,NODE>::insideTetrahedron2(const sxyz<T1>& p, const T2 nt) const {

        sxyz<T1> v1 = { nodes[ tetrahedra[nt].i[0] ].getX(),
            nodes[ tetrahedra[nt].i[0] ].getY(),
            nodes[ tetrahedra[nt].i[0] ].getZ() };

        sxyz<T1> v2 = { nodes[ tetrahedra[nt].i[1] ].getX(),
            nodes[ tetrahedra[nt].i[1] ].getY(),
            nodes[ tetrahedra[nt].i[1] ].getZ() };

        sxyz<T1> v3 = { nodes[ tetrahedra[nt].i[2] ].getX(),
            nodes[ tetrahedra[nt].i[2] ].getY(),
            nodes[ tetrahedra[nt].i[2] ].getZ() };

        sxyz<T1> v4 = { nodes[ tetrahedra[nt].i[3] ].getX(),
            nodes[ tetrahedra[nt].i[3] ].getY(),
            nodes[ tetrahedra[nt].i[3] ].getZ() };

        return sameSide(v1, v2, v3, v4, p) && sameSide(v2, v3, v4, v1, p) &&
        sameSide(v3, v4, v1, v2, p) && sameSide(v4, v1, v2, v3, p);
    }


    template<typename T1, typename T2, typename NODE>
    bool Grid3Duc<T1,T2,NODE>::insideTetrahedron(const sxyz<T1>& v, const T2 nt) const {


        // from http://steve.hollasch.net/cgindex/geometry/ptintet.html

        sxyz<T1> v1 = { nodes[ tetrahedra[nt].i[0] ].getX(),
            nodes[ tetrahedra[nt].i[0] ].getY(),
            nodes[ tetrahedra[nt].i[0] ].getZ() };

        sxyz<T1> v2 = { nodes[ tetrahedra[nt].i[1] ].getX(),
            nodes[ tetrahedra[nt].i[1] ].getY(),
            nodes[ tetrahedra[nt].i[1] ].getZ() };

        sxyz<T1> v3 = { nodes[ tetrahedra[nt].i[2] ].getX(),
            nodes[ tetrahedra[nt].i[2] ].getY(),
            nodes[ tetrahedra[nt].i[2] ].getZ() };

        sxyz<T1> v4 = { nodes[ tetrahedra[nt].i[3] ].getX(),
            nodes[ tetrahedra[nt].i[3] ].getY(),
            nodes[ tetrahedra[nt].i[3] ].getZ() };

        T1 D0 = det4(v1, v2, v3, v4);
        T1 D1 = det4( v, v2, v3, v4);
        T1 D2 = det4(v1,  v, v3, v4);
        T1 D3 = det4(v1, v2,  v, v4);
        T1 D4 = det4(v1, v2, v3,  v);

        int t1 = (signum(D0)==signum(D1));
        int t2 = (signum(D0)==signum(D2));
        int t3 = (signum(D0)==signum(D3));
        int t4 = (signum(D0)==signum(D4));

        bool it1, it2, it3, it4;
        it1 = it2 = it3 = it4 = 0;

        if ( std::abs(D1)<small2 ) {
            // points are coplanar, check if pt is inside triangle
            it1 = testInTriangle(v2, v3, v4, v);
        }
        if ( std::abs(D2)<small2 ) {
            it2 = testInTriangle(v1, v3, v4, v);
        }

        if ( std::abs(D3)<small2 ) {
            it3 = testInTriangle(v1, v2, v4, v);
        }

        if ( std::abs(D4)<small2 ) {
            it4 = testInTriangle(v1, v2, v3, v);
        }

        return ( t1 && t2 && t3 && t4 ) || it1 || it2 || it3 || it4;
    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::saveTT(const std::string &fname, const int all,
                                      const size_t nt, const int format) const {

        if ( format == 1 ) {
            std::string filename = fname+".dat";
            std::ofstream fout(filename.c_str());
            T2 nMax = nPrimary;
            if ( all == 1 ) {
                nMax = static_cast<T2>(nodes.size());
            }
            for ( T2 n=0; n<nMax; ++n ) {
                fout << nodes[n].getX() << '\t'
                << nodes[n].getY() << '\t'
                << nodes[n].getZ() << '\t'
                << nodes[n].getTT(nt) << '\n';
            }
            fout.close();
        } else if ( format == 2 ) {
#ifdef VTK

            std::string filename = fname+".vtu";

            vtkSmartPointer<vtkUnstructuredGrid> ugrid =
            vtkSmartPointer<vtkUnstructuredGrid>::New();

            vtkSmartPointer<vtkPoints> newPts =
            vtkSmartPointer<vtkPoints>::New();
            vtkSmartPointer<vtkDoubleArray> newScalars =
            vtkSmartPointer<vtkDoubleArray>::New();

            newScalars->SetName("Travel time");

            double xyz[3];
            T2 nMax = nPrimary;  // only primary are saved
            for (size_t n=0; n<nMax; ++n) {
                xyz[0] = nodes[n].getX();
                xyz[1] = nodes[n].getY();
                xyz[2] = nodes[n].getZ();
                newPts->InsertPoint(n, xyz);
                newScalars->InsertValue(n, nodes[n].getTT(nt) );
            }

            ugrid->SetPoints(newPts);
            ugrid->GetPointData()->SetScalars(newScalars);

            vtkSmartPointer<vtkTetra> tet =
            vtkSmartPointer<vtkTetra>::New();
            for (size_t n=0; n<tetrahedra.size(); ++n) {
                tet->GetPointIds()->SetId(0, tetrahedra[n].i[0] );
                tet->GetPointIds()->SetId(1, tetrahedra[n].i[1] );
                tet->GetPointIds()->SetId(2, tetrahedra[n].i[2] );
                tet->GetPointIds()->SetId(3, tetrahedra[n].i[3] );

                ugrid->InsertNextCell( tet->GetCellType(), tet->GetPointIds() );
            }
            vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
            vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();

            writer->SetFileName( filename.c_str() );
            //        writer->SetInputConnection( ugrid->GetProducerPort() );
            writer->SetInputData( ugrid );
            writer->SetDataModeToBinary();
            writer->Update();
#else
            std::cerr << "VTK not included during compilation.\nNothing saved.\n";
#endif
        } else if ( format == 3 ) {
            std::string filename = fname+".bin";
            std::ofstream fout(filename.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
            T2 nMax = nPrimary;
            if ( all == 1 ) {
                nMax = static_cast<T2>(nodes.size());
            }
            for ( T2 n=0; n<nMax; ++n ) {
                T1 tmp[] = { nodes[n].getX(), nodes[n].getY(), nodes[n].getZ(), nodes[n].getTT(nt) };
                fout.write( (char*)tmp, 4*sizeof(T1) );
            }
            fout.close();
        } else {
            throw std::runtime_error("Unsupported format for saving traveltimes");
        }
    }

#ifdef VTK
    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::saveModelVTU(const std::string &fname,
                                            const bool saveSlowness,
                                            const bool savePhysicalEntity) const {

        vtkSmartPointer<vtkUnstructuredGrid> ugrid =
        vtkSmartPointer<vtkUnstructuredGrid>::New();

        vtkSmartPointer<vtkPoints> newPts =
        vtkSmartPointer<vtkPoints>::New();

        double xyz[3];
        T2 nMax = nPrimary;  // only primary are saved
        for (size_t n=0; n<nMax; ++n) {
            xyz[0] = nodes[n].getX();
            xyz[1] = nodes[n].getY();
            xyz[2] = nodes[n].getZ();
            newPts->InsertPoint(n, xyz);
        }

        ugrid->SetPoints(newPts);

        vtkSmartPointer<vtkTetra> tet = vtkSmartPointer<vtkTetra>::New();
        vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
        if ( saveSlowness ) {
            data->SetName("Slowness");

            for (size_t n=0; n<tetrahedra.size(); ++n) {
                tet->GetPointIds()->SetId(0, tetrahedra[n].i[0] );
                tet->GetPointIds()->SetId(1, tetrahedra[n].i[1] );
                tet->GetPointIds()->SetId(2, tetrahedra[n].i[2] );
                tet->GetPointIds()->SetId(3, tetrahedra[n].i[3] );

                ugrid->InsertNextCell( tet->GetCellType(), tet->GetPointIds() );
                data->InsertNextValue( slowness[n] );
            }
        } else {
            data->SetName("Velocity");

            for (size_t n=0; n<tetrahedra.size(); ++n) {
                tet->GetPointIds()->SetId(0, tetrahedra[n].i[0] );
                tet->GetPointIds()->SetId(1, tetrahedra[n].i[1] );
                tet->GetPointIds()->SetId(2, tetrahedra[n].i[2] );
                tet->GetPointIds()->SetId(3, tetrahedra[n].i[3] );

                ugrid->InsertNextCell( tet->GetCellType(), tet->GetPointIds() );
                data->InsertNextValue( 1./slowness[n] );
            }
        }

        ugrid->GetCellData()->SetScalars(data);

        vtkSmartPointer<vtkIntArray> data_pe = vtkSmartPointer<vtkIntArray>::New();
        if ( savePhysicalEntity ) {
            data_pe->SetName("Physical entity");
            for (size_t n=0; n<tetrahedra.size(); ++n) {
                data_pe->InsertNextValue( tetrahedra[n].physical_entity );
            }
            ugrid->GetCellData()->AddArray(data_pe);
        }

        vtkSmartPointer<vtkXMLUnstructuredGridWriter> writer =
        vtkSmartPointer<vtkXMLUnstructuredGridWriter>::New();

        writer->SetFileName( fname.c_str() );
        writer->SetInputData( ugrid );
        writer->SetDataModeToBinary();
        writer->Update();

    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::saveModelVTR(const std::string &fname,
                                            const double *d,
                                            const bool saveSlowness) const {

        double x[] = { nodes[0].getX(), nodes[0].getX(),
            nodes[0].getY(), nodes[0].getY(),
            nodes[0].getZ(), nodes[0].getZ() };

        for (size_t n=1; n<nPrimary; ++n) {
            x[0] = x[0] < nodes[n].getX() ? x[0] : nodes[n].getX();
            x[1] = x[1] > nodes[n].getX() ? x[1] : nodes[n].getX();

            x[2] = x[2] < nodes[n].getY() ? x[2] : nodes[n].getY();
            x[3] = x[3] > nodes[n].getY() ? x[3] : nodes[n].getY();

            x[4] = x[4] < nodes[n].getZ() ? x[4] : nodes[n].getZ();
            x[5] = x[5] > nodes[n].getZ() ? x[5] : nodes[n].getZ();
        }

        int nn[3];
        nn[0] = 1 + (x[1]-x[0])/d[0];
        nn[1] = 1 + (x[3]-x[2])/d[1];
        nn[2] = 1 + (x[5]-x[4])/d[2];

        vtkSmartPointer<vtkDoubleArray> xCoords = vtkSmartPointer<vtkDoubleArray>::New();
        for (size_t n=0; n<nn[0]; ++n) {
            xCoords->InsertNextValue( x[0] + n*d[0] );
        }
        vtkSmartPointer<vtkDoubleArray> yCoords = vtkSmartPointer<vtkDoubleArray>::New();
        for (size_t n=0; n<nn[1]; ++n) {
            yCoords->InsertNextValue( x[2] + n*d[1] );
        }
        vtkSmartPointer<vtkDoubleArray> zCoords = vtkSmartPointer<vtkDoubleArray>::New();
        for (size_t n=0; n<nn[2]; ++n) {
            zCoords->InsertNextValue( x[4] + n*d[2] );
        }

        vtkSmartPointer<vtkRectilinearGrid> rgrid = vtkSmartPointer<vtkRectilinearGrid>::New();
        rgrid->SetDimensions( nn );
        rgrid->SetXCoordinates(xCoords);
        rgrid->SetYCoordinates(yCoords);
        rgrid->SetZCoordinates(zCoords);

        vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();

        if ( verbose>1 ) {
            std::cout << '\n';
        }
        sxyz<T1> pt;
        if ( saveSlowness ) {
            data->SetName("Slowness");
            for ( size_t n=0; n<rgrid->GetNumberOfCells(); ++n ) {
                if ( verbose>1 ) {
                    std::cout << "\r  Sampling cells: " << (100*n)/rgrid->GetNumberOfCells() << "%";
                    std::cout.flush();
                }
                rgrid->GetCell(n)->GetBounds(x);
                pt.x = 0.5*(x[0]+x[1]);
                pt.y = 0.5*(x[2]+x[3]);
                pt.z = 0.5*(x[4]+x[5]);

                for ( T2 nt=0; nt<tetrahedra.size(); ++nt ) {
                    if ( insideTetrahedron(pt, nt) ) {
                        data->InsertNextValue( slowness[nt] );
                        break;
                    }
                }
            }
        } else {
            data->SetName("Velocity");
            for ( size_t n=0; n<rgrid->GetNumberOfCells(); ++n ) {
                if ( verbose>1 ) {
                    std::cout << "\r  Sampling cells: " << (100*n)/rgrid->GetNumberOfCells() << "%";
                    std::cout.flush();
                }
                rgrid->GetCell(n)->GetBounds(x);
                pt.x = 0.5*(x[0]+x[1]);
                pt.y = 0.5*(x[2]+x[3]);
                pt.z = 0.5*(x[4]+x[5]);

                for ( T2 nt=0; nt<tetrahedra.size(); ++nt ) {
                    if ( insideTetrahedron(pt, nt) ) {
                        data->InsertNextValue( 1./slowness[nt] );
                        break;
                    }
                }
            }
        }
        if ( verbose>1 ) {
            std::cout << "\r  Sampling cells: 100%\n";
            std::cout.flush();
        }

        rgrid->GetCellData()->SetScalars( data );

        vtkSmartPointer<vtkXMLRectilinearGridWriter> writer =
        vtkSmartPointer<vtkXMLRectilinearGridWriter>::New();

        writer->SetFileName( fname.c_str() );
        //    writer->SetInputConnection( rgrid->GetProducerPort() );
        writer->SetInputData( rgrid );
        writer->SetDataModeToBinary();
        writer->Update();

    }
#endif

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::saveModelCRT(const std::string &fname,
                                            const double *d) const {

        double x[] = { nodes[0].getX(), nodes[0].getX(),
            nodes[0].getY(), nodes[0].getY(),
            nodes[0].getZ(), nodes[0].getZ() };

        for (size_t n=1; n<nPrimary; ++n) {
            x[0] = x[0] < nodes[n].getX() ? x[0] : nodes[n].getX();
            x[1] = x[1] > nodes[n].getX() ? x[1] : nodes[n].getX();

            x[2] = x[2] < nodes[n].getY() ? x[2] : nodes[n].getY();
            x[3] = x[3] > nodes[n].getY() ? x[3] : nodes[n].getY();

            x[4] = x[4] < nodes[n].getZ() ? x[4] : nodes[n].getZ();
            x[5] = x[5] > nodes[n].getZ() ? x[5] : nodes[n].getZ();
        }

        int nn[3];
        nn[0] = 1 + (x[1]-x[0])/d[0];
        nn[1] = 1 + (x[3]-x[2])/d[1];
        nn[2] = 1 + (x[5]-x[4])/d[2];

        if ( verbose ) {
            std::cout << "Model will be " << nn[0] << " x " << nn[1] << " x " << nn[2] << '\n';
        }
        sxyz<T1> pt;
        size_t nnn = nn[0]*nn[1]*nn[2];
        std::vector<T1> data( nnn );

        size_t nd=0;
        for ( size_t i3=0; i3<nn[2]; ++i3 ) {
            pt.z = x[4]+i3*d[2];
            for ( size_t i2=0; i2<nn[1]; ++i2 ) {
                pt.y = x[2]+i2*d[1];
                for ( size_t i1=0; i1<nn[0]; ++i1 ) {
                    pt.x = x[0]+i1*d[0];

                    if ( verbose>1 ) {
                        std::cout << "\r  Sampling cells: " << (100*nd)/nnn << '%';
                        std::cout.flush();
                    }

                    for ( T2 nt=0; nt<tetrahedra.size(); ++nt ) {
                        if ( insideTetrahedron(pt, nt) ) {
                            data[nd++] = 1./slowness[nt];
                            break;
                        }
                    }
                }
            }
        }
        if ( verbose>1 ) {
            std::cout << "\r  Sampling cells: 100%\n";
            std::cout.flush();
        }
        std::ofstream fout( fname.c_str() );
        fout << std::fixed;
        fout.precision(5);
        for ( size_t n=0; n<data.size(); ++n ) {
            fout << data[n] << ' ';
            if ( (n+1)%10 == 0) fout << '\n';
        }
        fout.close();
    }


    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::saveModelXYZ(const std::string &fname) const {

        std::ofstream fout(fname.c_str());
        for ( size_t n=0; n<tetrahedra.size(); ++n ) {
            fout << "> -Z" << slowness[n] << '\n';
            fout << nodes[ tetrahedra[n].i[0] ].getX() << ' '
            << nodes[ tetrahedra[n].i[0] ].getZ() << '\n';
            fout << nodes[ tetrahedra[n].i[1] ].getX() << ' '
            << nodes[ tetrahedra[n].i[1] ].getZ() << '\n';
            fout << nodes[ tetrahedra[n].i[2] ].getX() << ' '
            << nodes[ tetrahedra[n].i[2] ].getZ() << '\n';
        }
        fout.close();
    }



    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::localUpdate3D(NODE *vertexD,
                                             const size_t threadNo) const {

        // méthode of Lelievre et al. 2011

        T2 iA, iB, iC, iD;
        NODE *vertexA, *vertexB, *vertexC;

        for ( size_t no=0; no<vertexD->getOwners().size(); ++no ) {

            T2 tetNo = vertexD->getOwners()[no];

            for ( iD=0; iD<4; ++iD ) {
                if ( vertexD->getGridIndex() == tetrahedra[tetNo].i[iD] ) break;
            }

            iA = (iD+1)%4;
            iB = (iD+2)%4;
            iC = (iD+3)%4;
            vertexA = &(nodes[tetrahedra[tetNo].i[iA]]);
            vertexB = &(nodes[tetrahedra[tetNo].i[iB]]);
            vertexC = &(nodes[tetrahedra[tetNo].i[iC]]);

            if ( vertexA->getTT(threadNo) > vertexB->getTT(threadNo) ) {
                std::swap(iA, iB);
                std::swap(vertexA, vertexB);
            }
            if ( vertexA->getTT(threadNo) > vertexC->getTT(threadNo) ) {
                std::swap(iA, iC);
                std::swap(vertexA, vertexC);
            }

            if ( vertexA->getTT(threadNo) == std::numeric_limits<T1>::max() ) {
                continue;
            }

            T1 tABC = std::numeric_limits<T1>::max();

            if ( vertexB->getTT(threadNo) != std::numeric_limits<T1>::max() &&
                vertexC->getTT(threadNo) != std::numeric_limits<T1>::max() ) {

                // defined in text between eq 13 & eq 14
                T1 u = vertexB->getTT(threadNo) - vertexA->getTT(threadNo);
                T1 v = vertexC->getTT(threadNo) - vertexA->getTT(threadNo);

                sxyz<T1> v_b = { vertexC->getX() - vertexA->getX(),
                    vertexC->getY() - vertexA->getY(),
                    vertexC->getZ() - vertexA->getZ() };
                sxyz<T1> v_c = { vertexB->getX() - vertexA->getX(),
                    vertexB->getY() - vertexA->getY(),
                    vertexB->getZ() - vertexA->getZ() };

                T1 b = norm( v_b );
                T1 c = norm( v_c );
                T1 d2 = dot(v_b, v_c);  // eq 15

                T1 alpha = acos( d2 / (b*c) );

                T1 phi = c*b*sin(alpha);  // eq 23a

                T1 w_tilde = sqrt( slowness[tetNo]*slowness[tetNo]*phi*phi -
                                  u*u*b*b - v*v*c*c + 2.*u*v*d2 );  // eq 23b

                // project D on plane

                sxyz<T1> v_n = cross(v_b, v_c);

                T1 d_tmp = -vertexA->getX()*v_n.x - vertexA->getY()*v_n.y - vertexA->getZ()*v_n.z;

                T1 k = -(d_tmp + v_n.x*vertexD->getX() + v_n.y*vertexD->getY() + v_n.z*vertexD->getZ())/
                norm2(v_n);

                sxyz<T1> pt;
                pt.x = vertexD->getX() + k*v_n.x;
                pt.y = vertexD->getY() + k*v_n.y;
                pt.z = vertexD->getZ() + k*v_n.z;

                T1 rho0 = vertexD->getDistance( pt );

                sxyz<T1> v_pt = {pt.x-vertexA->getX(), pt.y-vertexA->getY(), pt.z-vertexA->getZ()};

                // project point on AB

                //                k = dot(v_pt,v_c)/dot(v_c,v_c);
                //                pt.x = vertexA->getX() + k*v_c.x;
                //                pt.y = vertexA->getY() + k*v_c.y;
                //                pt.z = vertexA->getZ() + k*v_c.z;
                //
                //                T1 xi0 = vertexA->getDistance( pt )/c;
                T1 xi0 = dot(v_pt,v_c)/dot(v_c,v_c);

                // project point on AC

                //                k = dot(v_pt,v_b)/dot(v_b,v_b);
                //                pt.x = vertexA->getX() + k*v_b.x;
                //                pt.y = vertexA->getY() + k*v_b.y;
                //                pt.z = vertexA->getZ() + k*v_b.z;
                //
                //                T1 zeta0 = vertexA->getDistance( pt )/b;
                T1 zeta0 = dot(v_pt,v_b)/dot(v_b,v_b);


                T1 beta = u*b*b - v*d2;    // from eq 19
                T1 gamma = v*c*c - u*d2;

                T1 xi_tilde = -std::abs(beta)*rho0/(phi*w_tilde);      // eq 22a
                T1 zeta_tilde = -std::abs(gamma)*rho0/(phi*w_tilde);   // eq 22b

                T1 xi = xi_tilde + xi0;         // defined in text between eq 13 & eq 14
                T1 zeta = zeta_tilde + zeta0;

                if ( 0.<xi && xi<1. && 0.<zeta && zeta<1. && 0.<(xi+zeta) && (xi+zeta)<1. ) {
                    // eq 25
                    tABC = vertexA->getTT(threadNo) + u*xi0 + v*zeta0 + w_tilde*rho0/phi;
                }
            }

            T1 t = vertexA->getTT(threadNo) + slowness[tetNo] * vertexD->getDistance( *vertexA );
            if ( t < tABC ) tABC = t;
            t = vertexB->getTT(threadNo) + slowness[tetNo] * vertexD->getDistance( *vertexB );
            if ( t < tABC ) tABC = t;
            t = vertexC->getTT(threadNo) + slowness[tetNo] * vertexD->getDistance( *vertexC );
            if ( t < tABC ) tABC = t;

            t = localUpdate2D(vertexA, vertexB, vertexD, tetNo, threadNo);
            if ( t < tABC ) tABC = t;
            t = localUpdate2D(vertexA, vertexC, vertexD, tetNo, threadNo);
            if ( t < tABC ) tABC = t;
            t = localUpdate2D(vertexB, vertexC, vertexD, tetNo, threadNo);
            if ( t < tABC ) tABC = t;

            if ( tABC<vertexD->getTT(threadNo) )
                vertexD->setTT(tABC, threadNo);

        }
    }


    template<typename T1, typename T2, typename NODE>
    T1 Grid3Duc<T1,T2,NODE>::localUpdate2D(const NODE *vertexA,
                                           const NODE *vertexB,
                                           const NODE *vertexC,
                                           const T2 tetNo,
                                           const size_t threadNo) const {

        if ( vertexB->getTT(threadNo)==std::numeric_limits<T1>::max() &&
            vertexA->getTT(threadNo)==std::numeric_limits<T1>::max() ) {
            return std::numeric_limits<T1>::max();
        }
        T1 t;

        T1 u = vertexB->getTT(threadNo) - vertexA->getTT(threadNo);

        sxyz<T1> v_b = { vertexC->getX() - vertexA->getX(),
            vertexC->getY() - vertexA->getY(),
            vertexC->getZ() - vertexA->getZ() };
        sxyz<T1> v_c = { vertexB->getX() - vertexA->getX(),
            vertexB->getY() - vertexA->getY(),
            vertexB->getZ() - vertexA->getZ() };

        T1 c = norm( v_c );

        T1 w2 = slowness[tetNo]*slowness[tetNo]*c*c - u*u;
        if ( w2 < 0.0 ) {
            return std::numeric_limits<T1>::max();
        }

        T1 w = sqrt( w2 );

        T1 k = dot(v_b,v_c)/dot(v_c,v_c);
        sxyz<T1> pt;
        pt.x = vertexA->getX() + k*v_c.x;
        pt.y = vertexA->getY() + k*v_c.y;
        pt.z = vertexA->getZ() + k*v_c.z;

        T1 rho0 = vertexC->getDistance( pt );
        //        T1 xi0 = vertexA->getDistance( pt )/c;
        T1 xi0 = k;

        T1 xi = xi0 - u*rho0/(w*c);

        if ( 0.<xi && xi<1. ) {
            t = vertexA->getTT(threadNo) + u*xi0 + w*rho0/c;
        } else {
            t = std::numeric_limits<T1>::max();
        }

        return t;
    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::local3Dsolver(NODE *vertexD,
                                             const size_t threadNo) const {

        // Méthode de Qian et al. 2007

        T2 iA, iB, iC, iD;
        NODE *vertexA, *vertexB, *vertexC;
        T1 AB, AC;

        for ( size_t no=0; no<vertexD->getOwners().size(); ++no ) {

            T2 tetNo = vertexD->getOwners()[no];

            for ( iD=0; iD<4; ++iD ) {
                if ( vertexD->getGridIndex() == tetrahedra[tetNo].i[iD] ) break;
            }

            iA = (iD+1)%4;
            iB = (iD+2)%4;
            iC = (iD+3)%4;
            vertexA = &(nodes[tetrahedra[tetNo].i[iA]]);
            vertexB = &(nodes[tetrahedra[tetNo].i[iB]]);
            vertexC = &(nodes[tetrahedra[tetNo].i[iC]]);

            if ( vertexA->getTT(threadNo) > vertexB->getTT(threadNo) ) {
                std::swap(iA, iB);
                std::swap(vertexA, vertexB);
            }
            if ( vertexA->getTT(threadNo) > vertexC->getTT(threadNo) ) {
                std::swap(iA, iC);
                std::swap(vertexA, vertexC);
            }

            if ( vertexA->getTT(threadNo) == std::numeric_limits<T1>::max() ) {
                continue;
            }

            AB = vertexA->getDistance( *vertexB );
            AC = vertexA->getDistance( *vertexC );

            bool apply2Dsolvers = true;

            if (std::abs(vertexB->getTT(threadNo)-vertexA->getTT(threadNo))<=AB*slowness[tetNo] &&
                std::abs(vertexC->getTT(threadNo)-vertexA->getTT(threadNo))<=AC*slowness[tetNo]) {

                // Qian et al, 2007, eq 2.3

                T1 ab[4], ac[4], n[2][3];

                // vec(AB)
                ab[0] = vertexB->getX()-vertexA->getX();
                ab[1] = vertexB->getY()-vertexA->getY();
                ab[2] = vertexB->getZ()-vertexA->getZ();

                ab[3] = (vertexB->getTT(threadNo)-vertexA->getTT(threadNo)) / slowness[tetNo];

                // vec(AC)
                ac[0] = vertexC->getX()-vertexA->getX();
                ac[1] = vertexC->getY()-vertexA->getY();
                ac[2] = vertexC->getZ()-vertexA->getZ();

                ac[3] = (vertexC->getTT(threadNo)-vertexA->getTT(threadNo)) / slowness[tetNo];

                int rv = solveEq23(ab, ac, n);

                if ( rv == 1 ) {

                    for ( size_t ns=0; ns<2; ++ns ) {
                        //
                        // find pt E
                        //

                        // plane vec(AB) cross vec(AC) passing by A: ax + by + cz + d = 0

                        T1 a = ab[1]*ac[2] - ac[1]*ab[2];
                        T1 b = ac[0]*ab[2] - ab[0]*ac[2];
                        T1 c = ab[0]*ac[1] - ac[0]*ab[1];

                        T1 d = -vertexA->getX()*a - vertexA->getY()*b - vertexA->getZ()*c;

                        //				T1 k = -(d + a*vertexA->getX() + b*vertexA->getY() + c*vertexA->getZ())/
                        //				(a*n[0] + b*n[1] + c*n[2]);
                        T1 k = -(d + a*vertexD->getX() + b*vertexD->getY() + c*vertexD->getZ())/  // TODO check here if vertex D
                        (a*n[ns][0] + b*n[ns][1] + c*n[ns][2]);

                        sxyz<T1> E;
                        E.x = vertexD->getX() + k*n[ns][0];
                        E.y = vertexD->getY() + k*n[ns][1];
                        E.z = vertexD->getZ() + k*n[ns][2];




                        //					// area of ABC
                        //					T1 ABC = 0.5*sqrt(a*a + b*b + c*c);
                        //
                        //					// area of EAB
                        //					sxyz<T1> v1, v2;
                        //					// vec(EA)
                        //					v1.x = vertexA->getX() - E.x;
                        //					v1.y = vertexA->getY() - E.y;
                        //					v1.z = vertexA->getZ() - E.z;
                        //					// vec(EB)
                        //					v2.x = vertexB->getX() - E.x;
                        //					v2.y = vertexB->getY() - E.y;
                        //					v2.z = vertexB->getZ() - E.z;
                        //
                        //					sxyz<T1> v3 = cross(v1, v2);
                        //
                        //					T1 EAB = 0.5 * norm(v3);
                        //
                        //					// area of EAC
                        //					// vec(EC)
                        //					v2.x = vertexC->getX() - E.x;
                        //					v2.y = vertexC->getY() - E.y;
                        //					v2.z = vertexC->getZ() - E.z;
                        //
                        //					v3 = cross(v1, v2);
                        //
                        //					T1 EAC = 0.5 * norm(v3);
                        //
                        //					// area of EBC
                        //					// vec(EB)
                        //					v1.x = vertexB->getX() - E.x;
                        //					v1.y = vertexB->getY() - E.y;
                        //					v1.z = vertexB->getZ() - E.z;
                        //
                        //					v3 = cross(v1, v2);
                        //
                        //					T1 EBC = 0.5 * norm(v3);
                        //
                        //					if ( std::abs(EAB+EAC+EBC-ABC)<small ) {

                        if ( testInTriangle(vertexA, vertexB, vertexC, E) ) {

                            // find point on wavefront plane

                            a = n[ns][0];
                            b = n[ns][1];
                            c = n[ns][2];
                            d = -vertexA->getX()*a - vertexA->getY()*b - vertexA->getZ()*c;

                            k = -(d + a*vertexD->getX() + b*vertexD->getY() + c*vertexD->getZ())/
                            (a*n[ns][0] + b*n[ns][1] + c*n[ns][2]);

                            sxyz<T1> pt;
                            pt.x = vertexD->getX() + k*n[ns][0];
                            pt.y = vertexD->getY() + k*n[ns][1];
                            pt.z = vertexD->getZ() + k*n[ns][2];


                            //						std::cout << "\nplot3("<< vertexA->getX() << "," << vertexA->getY() << "," << vertexA->getZ() << ",'o'), hold on\n";
                            //						std::cout << "plot3("<< vertexB->getX() << "," << vertexB->getY() << "," << vertexB->getZ() << ",'o')\n";
                            //						std::cout << "plot3("<< vertexC->getX() << "," << vertexC->getY() << "," << vertexC->getZ() << ",'o')\n";
                            //						std::cout << "plot3("<< vertexD->getX() << "," << vertexD->getY() << "," << vertexD->getZ() << ",'ko')\n";
                            //						std::cout << "plot3("<< E.x << "," << E.y << "," << E.z << ",'go')\n";
                            //						std::cout << "plot3("<< pt.x << "," << pt.y << "," << pt.z << ",'ro')\n";
                            //						std::cout << "plot3(["<< pt.x << " "<< pt.x+0.1*n[ns][0]<< "],[" << pt.y << " "<< pt.y+0.1*n[ns][1]<< "],[" << pt.z << " "<< pt.z+0.1*n[ns][2]<< "],'r'), axis equal\n";


                            sxyz<T1> AD;
                            AD.x = vertexD->getX() - vertexA->getX();
                            AD.y = vertexD->getY() - vertexA->getY();
                            AD.z = vertexD->getZ() - vertexA->getZ();

                            T1 d2 = vertexD->getDistance( E );
                            T1 d3 = vertexD->getDistance( pt );
                            T1 d4 = std::abs( AD.x*n[ns][0] + AD.y*n[ns][1] + AD.z*n[ns][2] );

                            if ( std::abs(d3-d4)>small ) {
                                std::cout << " d3 ne d4: " << d3 << '\t' << d4 << '\t' << d2 << '\n';
                            }

                            T1 t = vertexA->getTT(threadNo) +
                            d3*slowness[tetNo];

                            if ( t<vertexD->getTT(threadNo) )
                                vertexD->setTT(t, threadNo);

                            apply2Dsolvers = false;
                            break;
                        }
                    }
                }
            }

            if ( apply2Dsolvers ) {
                T1 tABD = local2Dsolver(vertexA, vertexB, vertexD, tetNo, threadNo);
                T1 tACD = local2Dsolver(vertexA, vertexC, vertexD, tetNo, threadNo);
                T1 tBCD = local2Dsolver(vertexB, vertexC, vertexD, tetNo, threadNo);

                T1 t = tABD < tACD ? tABD : tACD;
                t = t < tBCD ? t : tBCD;

                if ( t<vertexD->getTT(threadNo) )
                    vertexD->setTT(t, threadNo);
            }

        }
    }

    template<typename T1, typename T2, typename NODE>
    T1 Grid3Duc<T1,T2,NODE>::local2Dsolver(const NODE *vertexA,
                                           const NODE *vertexB,
                                           const NODE *vertexC,
                                           const T2 tetraNo,
                                           const size_t threadNo) const {
        static const double pi2 = pi / 2.;

        if ( vertexB->getTT(threadNo)==std::numeric_limits<T1>::max() &&
            vertexA->getTT(threadNo)==std::numeric_limits<T1>::max() ) {
            return std::numeric_limits<T1>::max();
        }

        T1 t;

        T1 a = vertexB->getDistance( *vertexC );
        T1 b = vertexA->getDistance( *vertexC );
        T1 c = vertexA->getDistance( *vertexB );
        if ( std::abs(vertexB->getTT(threadNo)-vertexA->getTT(threadNo))<= c*slowness[tetraNo] ) {

            T1 theta = asin( std::abs(vertexB->getTT(threadNo)-vertexA->getTT(threadNo))/
                            (c*slowness[tetraNo]) );

            T1 gamma = acos((a*a + b*b - c*c)/(2.*a*b));

            if ( gamma > pi2 ) {
                std::cout << "*** Obtuse angle: " << gamma*57.2957795 << " ***\n";
            } else {
                std::cout << "Accute angle: " << gamma*57.2957795 << " \n";
            }

            T1 beta  = acos((b*b + c*c - a*a)/(2.*b*c));
            T1 alpha = acos((a*a + c*c - b*b)/(2.*a*c));

            if ( ((0.>alpha-pi2?0.:alpha-pi2)<=theta && theta<=(pi2-beta) ) ||
                ((alpha-pi2)<=theta && theta<=(0.<pi2-beta?0.:pi2-beta)) ) {
                T1 h = a*sin(alpha-theta);
                T1 H = b*sin(beta+theta);
                t = 0.5*(h*slowness[tetraNo] + vertexB->getTT(threadNo)) +
                0.5 *(H*slowness[tetraNo] + vertexA->getTT(threadNo));

            } else {
                t = vertexA->getTT(threadNo) + b*slowness[tetraNo];
                t = t < vertexB->getTT(threadNo) + a*slowness[tetraNo] ? t : vertexB->getTT(threadNo) + a*slowness[tetraNo];
            }
        } else {
            t = vertexA->getTT(threadNo) + b*slowness[tetraNo];
            t = t < vertexB->getTT(threadNo) + a*slowness[tetraNo] ? t : vertexB->getTT(threadNo) + a*slowness[tetraNo];
        }
        t = t<vertexC->getTT(threadNo) ? t : vertexC->getTT(threadNo);

        return t;
    }


    template<typename T1, typename T2, typename NODE>
    int Grid3Duc<T1,T2,NODE>::solveEq23(const T1 a[], const T1 b[], T1 n[][3]) const {
        // returns 0 if no solution
        //         1 if solutions exist

        T1 a02 = a[0]*a[0];
        T1 a12 = a[1]*a[1];
        T1 a22 = a[2]*a[2];
        T1 a32 = a[3]*a[3];
        T1 b02 = b[0]*b[0];
        T1 b12 = b[1]*b[1];
        T1 b22 = b[2]*b[2];
        T1 b32 = b[3]*b[3];
        T1 a23 = a[2]*a[2]*a[2];

        T1 s1 = (a[2]*b[1] - a[1]*b[2])*(a[2]*b[1] - a[1]*b[2])*
        (a02*(b12 + b22) - a32*(b02 + b12 + b22) + 2*a[0]*a[3]*b[0]*b[3] - a02*b32 -
         2*a[1]*b[1]*(a[0]*b[0] + a[2]*b[2] - a[3]*b[3]) + 2*a[2]*b[2]*
         (-(a[0]*b[0]) + a[3]*b[3]) + a22*(b02 + b12 - b32) + a12*(b02 + b22 - b32));

        if ( s1 < 0.0 ) {
            return 0;
        } else {

            T1 d1 = (a22*(b02 + b12) - 2*a[0]*a[2]*b[0]*b[2] -
                     2*a[1]*b[1]*(a[0]*b[0] + a[2]*b[2])  +
                     a12*(b02 + b22) + a02*(b12 + b22));
            T1 d2 = ((a[2]*b[1] - a[1]*b[2]) *(a22*(b02 + b12) -
                                               2*a[0]*a[2]*b[0]*b[2] -
                                               2*a[1]*b[1]*(a[0]*b[0] + a[2]*b[2]) +
                                               a12*(b02 + b22) + a02*(b12 + b22)));

            if ( d1==0.0 || d2==0.0 ) return 0;

            s1 = sqrt(s1);

            n[0][0] = (a[0]*a[3]*(b12 + b22) + a12*b[0]*b[3] - a[0]*a[2]*b[2]*b[3] -
                       a[1]*b[1]*(a[3]*b[0] + a[0]*b[3]) +
                       a[2]*b[0]*(-(a[3]*b[2]) + a[2]*b[3])  - s1) / d1;

            n[0][1] = (a[2]*a[3]*b[1]*(-(a[0]*b[0]*b[1])  + a[1]*(b02 + 2*b22)) +
                       a23*b12*b[3] + a[2]*(a[0]*b[1]*(-(a[1]*b[0])  + a[0]*b[1]) +
                                            a12*b22)*b[3] -
                       a22*b[1]*b[2]*(a[3]*b[1] + 2*a[1]*b[3]) -
                       a[1]*b[2]*(a[1]*a[3]*(b02 + b22) - a[0]*a[1]*b[0]*b[3] +
                                  a[0]*b[1]*(-(a[3]*b[0])  + a[0]*b[3]) ) +
                       a[2]*b[0]*s1 - a[0]*b[2]*s1) / d2;

            n[0][2] = (a[1]*b22*(a[3]*(a[0]*b[0] + a[1]*b[1])  - (a02 + a12)*b[3]) +
                       a22*b[1]*(a[3]*(b02 + b12) - (a[0]*b[0] + a[1]*b[1]) *b[3]) +
                       a[2]*b[2]*(-(a[1]*a[3]*(b02 + 2*b12)) + a[0]*a[1]*b[0]*b[3] +
                                  2*a12*b[1]*b[3] + a[0]*b[1]*(-(a[3]*b[0]) + a[0]*b[3]) ) -
                       a[1]*b[0]*s1 + a[0]*b[1]*s1)/ d2;

            n[1][0] = (a[0]*a[3]*(b12 + b22) + a12*b[0]*b[3] - a[0]*a[2]*b[2]*b[3] -
                       a[1]*b[1]*(a[3]*b[0] + a[0]*b[3]) +
                       a[2]*b[0]*(-(a[3]*b[2]) + a[2]*b[3])  + s1) / d1;

            n[1][1] = (a[2]*a[3]*b[1]*(-(a[0]*b[0]*b[1])  + a[1]*(b02 + 2*b22)) +
                       a23*b12*b[3] + a[2]*(a[0]*b[1]*(-(a[1]*b[0])  + a[0]*b[1]) +
                                            a12*b22)*b[3] - a22*b[1]*b[2]*(a[3]*b[1] +
                                                                           2*a[1]*b[3]) -
                       a[1]*b[2]*(a[1]*a[3]*(b02 + b22) - a[0]*a[1]*b[0]*b[3] +
                                  a[0]*b[1]*(-(a[3]*b[0])  + a[0]*b[3]) ) -
                       a[2]*b[0]*s1 + a[0]*b[2]*s1) / d2;

            n[1][2] = (a[1]*b22*(a[3]*(a[0]*b[0] + a[1]*b[1])  - (a02 + a12)*b[3]) +
                       a22*b[1]*(a[3]*(b02 + b12) - (a[0]*b[0] + a[1]*b[1]) *b[3]) +
                       a[2]*b[2]*(-(a[1]*a[3]*(b02 + 2*b12)) + a[0]*a[1]*b[0]*b[3] +
                                  2*a12*b[1]*b[3] + a[0]*b[1]*(-(a[3]*b[0])  + a[0]*b[3]) ) +
                       a[1]*b[0]*s1 - a[0]*b[1]*s1) / d2;
        }
        return 1;
    }


    template<typename T1, typename T2, typename NODE>
    T1 Grid3Duc<T1,T2,NODE>::getTraveltimeFromRaypath(const std::vector<sxyz<T1>>& Tx,
                                                      const std::vector<T1>& t0,
                                                      const sxyz<T1> &Rx,
                                                      const size_t threadNo) const {
        T1 tt = 0.0;

        T1 minDist = small;

        for ( size_t ns=0; ns<Tx.size(); ++ns ) {
            if ( Rx == Tx[ns] ) {
                return t0[ns];
            }
        }

        std::vector<bool> txOnNode( Tx.size(), false );
        std::vector<bool> txOnEdge( Tx.size(), false );
        std::vector<bool> txOnFace( Tx.size(), false );
        std::vector<T2> txNode( Tx.size() );
        std::vector<T2> txCell( Tx.size() );
        std::vector<std::array<T2,2>> txEdges( Tx.size() );
        std::vector<std::array<T2,3>> txFaces( Tx.size() );
        std::vector<std::vector<T2>> txNeighborCells( Tx.size() );
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            for ( T2 nn=0; nn<nodes.size(); ++nn ) {
                if ( nodes[nn].isPrimary() ) {
                    if ( nodes[nn] == Tx[nt] ) {
                        txOnNode[nt] = true;
                        txNode[nt] = nn;
                        break;
                    }
                }
            }
        }
#ifdef DEBUG_RP
        std::cout << "\n\n\n*** RP debug data - Source\n";
        std::vector<std::vector<sxyz<T1>>> r_data(1);
        r_data[0].push_back(Rx);
#endif
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            if ( !txOnNode[nt] ) {
                txCell[nt] = getCellNo( Tx[nt] );

                std::array<T2,4> itmp = getPrimary(txCell[nt]);
                // find adjacent cells
                const T2 ind[6][2] = {
                    {itmp[0], itmp[1]},
                    {itmp[0], itmp[2]},
                    {itmp[0], itmp[3]},
                    {itmp[1], itmp[2]},
                    {itmp[1], itmp[3]},
                    {itmp[2], itmp[3]} };

                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    for ( auto nc0=nodes[ind[nedge][0]].getOwners().begin(); nc0!=nodes[ind[nedge][0]].getOwners().end(); ++nc0 ) {
                        if ( std::find(nodes[ind[nedge][1]].getOwners().begin(), nodes[ind[nedge][1]].getOwners().end(), *nc0)!=nodes[ind[nedge][1]].getOwners().end() )
                            txNeighborCells[nt].push_back( *nc0 );
                    }
                }
                // check if on edge
                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    if ( distSqPointToSegment( &nodes[ind[nedge][0]], &nodes[ind[nedge][1]], Tx[nt]) < small2 ) {
                        txOnEdge[nt] = true;
                        txEdges[nt][0] = ind[nedge][0];
                        txEdges[nt][1] = ind[nedge][1];
                        break;
                    }
                }
                if ( !txOnEdge[nt] ) {
                    // check if on face
                    const T2 indf[4][3] = {
                        {itmp[0], itmp[1], itmp[2]},
                        {itmp[0], itmp[1], itmp[3]},
                        {itmp[0], itmp[2], itmp[3]},
                        {itmp[1], itmp[2], itmp[3]} };
                    for ( size_t nface=0; nface<4; ++nface ) {
                        if ( testInTriangle(&nodes[indf[nface][0]], &nodes[indf[nface][1]], &nodes[indf[nface][2]], Tx[nt]) ) {
                            txOnFace[nt] = true;
                            txFaces[nt][0] = indf[nface][0];
                            txFaces[nt][1] = indf[nface][1];
                            txFaces[nt][2] = indf[nface][2];
                            break;
                        }
                    }
                }
            }
#ifdef DEBUG_RP
            std::cout << "   src no: " << nt << '\n';
            if ( txOnNode[nt] ) {
                std::cout << "     onNode\n"
                << "\t    txNode: " << txNode[nt] << '\n'
                << "\t    coords: " << nodes[txNode[nt]] << '\n';
            } else if (txOnEdge[nt] ) {
                std::cout << "     onEdge\n"
                << "\t    edge vertices no: " << txEdges[nt][0] << ' ' << txEdges[nt][1] << '\n'
                << "\t  vertices: " << nodes[txEdges[nt][0]] << '\n'
                << "\t          : " << nodes[txEdges[nt][1]] << '\n';
            } else if (txOnEdge[nt] ) {
                std::cout << "     onFace\n"
                << "\t    face vertices no: " << txFaces[nt][0] << ' ' << txFaces[nt][1] << ' ' << txFaces[nt][2] << '\n'
                << "\t  vertices: " << nodes[txFaces[nt][0]] << '\n'
                << "\t          : " << nodes[txFaces[nt][1]] << '\n'
                << "\t          : " << nodes[txFaces[nt][2]] << '\n';
            } else {
                std::array<T2,4> itmp = getPrimary(txCell[nt]);
                std::cout << "     inCell\n"
                << "\t    cellNo: " << txCell[nt] << '\n'
                << "\t  vertices: " << nodes[itmp[0]] << '\n'
                << "\t          : " << nodes[itmp[1]] << '\n'
                << "\t          : " << nodes[itmp[2]] << '\n'
                << "\t          : " << nodes[itmp[3]] << '\n';
            }
            std::cout << '\n';
#endif
        }

        T2 cellNo=0, nodeNo=0;
        sxyz<T1> curr_pt( Rx ), prev_pt( Rx );
        bool atRx = true;

        bool onNode = false;
        bool onEdge = false;
        bool onFace = false;
        std::array<T2,2> edgeNodes{ {0, 0} };
        std::array<T2,3> faceNodes{ {0, 0, 0} };
        std::array<T2,3> faceNodesStart{ {0, 0, 0} };

        Grad3D<T1,NODE>* grad3d = nullptr;
        if ( rp_method == 0 ) {
            grad3d = new Grad3D_ls_fo<T1,NODE>();
        } else if ( rp_method == 1 ) {
            grad3d = new Grad3D_ls_so<T1,NODE>();
        } else if ( rp_method == 2 ) {
            grad3d = new Grad3D_ab<T1,NODE>();
        }
        bool reachedTx = false;

        for ( T2 nn=0; nn<nodes.size(); ++nn ) {
            if ( nodes[nn].getDistance( curr_pt ) < small ) {
                nodeNo = nn;
                onNode = true;
                break;
            }
        }
        if ( !onNode ) {
            cellNo = getCellNo( curr_pt );

            std::array<T2,4> itmp = getPrimary(cellNo);
            T2 ind[6][2] = {
                {itmp[0], itmp[1]},
                {itmp[0], itmp[2]},
                {itmp[0], itmp[3]},
                {itmp[1], itmp[2]},
                {itmp[1], itmp[3]},
                {itmp[2], itmp[3]} };

            for ( size_t n=0; n<6; ++n ) {
                if ( areCollinear(curr_pt, nodes[ind[n][0]], nodes[ind[n][1]]) ) {
                    onEdge = true;
                    edgeNodes[0] = ind[n][0];
                    edgeNodes[1] = ind[n][1];
                    break;
                }
            }
        }
        if ( !onNode && !onEdge ) {
            std::array<T2,4> itmp = getPrimary(cellNo);
            std::array<T2,3> ind[4] = {
                {itmp[0], itmp[1], itmp[2]},
                {itmp[0], itmp[1], itmp[3]},
                {itmp[0], itmp[2], itmp[3]},
                {itmp[1], itmp[2], itmp[3]}};
            for ( size_t n=0; n<4; ++n ) {
                std::sort( ind[n].begin(), ind[n].end() );
            }

            for ( size_t n=0; n<4; ++n ) {
                if ( areCoplanar(curr_pt, nodes[ind[n][0]], nodes[ind[n][1]], nodes[ind[n][2]]) ) {
                    onFace = true;
                    faceNodesStart = ind[n];
                    break;
                }
            }
        }

        for ( size_t nt=0; nt<txCell.size(); ++nt ) {
            if ( cellNo == txCell[nt] ) {
                tt += t0[nt] + slowness[cellNo] * curr_pt.getDistance( Tx[nt] );
                reachedTx = true;
                break;
            }
        }

        sxyz<T1> g;
        while ( reachedTx == false ) {
            if ( onNode ) {
#ifdef DEBUG_RP
                printRaypathData(curr_pt, g, onNode, onEdge, onFace, cellNo,
                                 nodeNo, edgeNodes, faceNodes);
#endif
                if ( rp_method < 2 ) {
                    // find cells common to edge
                    std::set<NODE*> nnodes;
                    for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                        getNeighborNodes(*nc, nnodes);
                    }
                    // compute gradient with nodes from all common cells
                    g = grad3d->compute(curr_pt, nodes[nodeNo].getTT(threadNo), nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(1);
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    ref_pt[0] = &(nodes[nodeNo]);
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }

                // find cell for which gradient intersect opposing face
                bool foundIntersection = false;
                for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {

                    std::array<T2,3> nb;
                    size_t n=0;
                    for (auto nn=this->neighbors[*nc].begin(); nn!=this->neighbors[*nc].end(); ++nn ) {
                        if ( *nn != nodeNo && nodes[*nn].isPrimary() ) {
                            nb[n++] = *nn;
                        }
                    }
                    std::sort(nb.begin(), nb.end());

                    foundIntersection = intersectVecTriangle( nodeNo, g, nb[0], nb[1], nb[2], curr_pt);
                    if ( !foundIntersection ) {
                        continue;
                    }

                    //  check if cell is (one of) TxCell(s)
                    for (size_t nt=0; nt<Tx.size(); ++nt) {
                        if ( *nc == txCell[nt] ) {
                            tt += t0[nt] + slowness[*nc] * nodes[nodeNo].getDistance( Tx[nt] );
#ifdef DEBUG_RP
                            r_data[0].push_back(Tx[nt]);
#endif
                            reachedTx = true;
                            break;
                        }
                    }
                    if ( reachedTx ) {
                        break;
                    }

                    bool break_flag = check_pt_location(curr_pt, nb, onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);
#ifdef DEBUG_RP
                    r_data[0].push_back(curr_pt);
#endif
                    tt += slowness[*nc] * prev_pt.getDistance( curr_pt );
                    prev_pt = curr_pt;

                    if ( break_flag ) break;

                    // find next cell
                    cellNo = findAdjacentCell1(faceNodes, nodeNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        tt = 0.0;
                        reachedTx = true;
                    }
                    break;
                }

                if ( foundIntersection == false ) {
#ifdef DEBUG_RP
                    std::cout << "\n\nWarning: raypath (onNode) likely going outside mesh for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << '\n';
                    std::cout << "         Projecting gradient on external face and resuming" << std::endl;
#endif
                    // projet gradient on face and find intersection on next edge
                    sxyz<T1> pt_i;
                    foundIntersection = projectOnFace(curr_pt, nodeNo, g, edgeNodes, pt_i);

                    if ( foundIntersection == false ) {
                        std::cout << "\n\nWarning: finding raypath (onNode) failed to converge for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        tt = 0.0;
                        reachedTx = true;
                        break;
                    }
                    // find which cell we are in
                    for ( size_t nc=0; nc<tetrahedra.size(); ++nc ) {
                        std::array<T2,4> itet = {tetrahedra[nc].i[0],
                            tetrahedra[nc].i[1],
                            tetrahedra[nc].i[2],
                            tetrahedra[nc].i[3]};
                        // because we are at the surface, there is only one cell with nodeNo & edgeNodes
                        if (std::find(itet.begin(), itet.end(), nodeNo) != itet.end() &&
                            std::find(itet.begin(), itet.end(), edgeNodes[0]) != itet.end() &&
                            std::find(itet.begin(), itet.end(), edgeNodes[1]) != itet.end()) {
                            cellNo = static_cast<T2>(nc);
                            break;
                        }
                    }

                    //  check if cell is (one of) TxCell(s)
                    for (size_t nt=0; nt<Tx.size(); ++nt) {
                        if ( cellNo == txCell[nt] ) {
                            tt += t0[nt] + slowness[cellNo] * nodes[nodeNo].getDistance( Tx[nt] );
#ifdef DEBUG_RP
                            r_data[0].push_back(Tx[nt]);
#endif
                            reachedTx = true;
                            break;
                        }
                    }
                    if ( reachedTx ) {
                        break;
                    }
                    T2 prevNode = nodeNo;

                    // we might be on one of the nodes
                    if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[0];
                        pt_i = nodes[edgeNodes[0]];
                        onEdge = false;
                    } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[1];
                        pt_i = nodes[edgeNodes[1]];
                        onEdge = false;
                    } else {
                        onEdge = true;
                        onNode = false;
                    }

                    T1 dt;
                    // find traversed cell
                    if ( onNode ) {
                        // propagation was along edge
                        dt = nodes[nodeNo].getTT(threadNo) - nodes[prevNode].getTT(threadNo);
                    } else { // onEdge
                        T2 traversedCell = 0;
                        for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                            if ( std::find( nodes[edgeNodes[0]].getOwners().begin(), nodes[edgeNodes[0]].getOwners().end(), *nc ) != nodes[edgeNodes[0]].getOwners().end() &&
                                std::find( nodes[edgeNodes[1]].getOwners().begin(), nodes[edgeNodes[1]].getOwners().end(), *nc ) != nodes[edgeNodes[1]].getOwners().end() ) {
                                traversedCell = *nc;
                                break;
                            }
                        }
                        dt = slowness[traversedCell] * prev_pt.getDistance( curr_pt );
                    }

                    curr_pt = pt_i;
#ifdef DEBUG_RP
                    r_data[0].push_back(curr_pt);
#endif
                    tt += dt;
                    prev_pt = curr_pt;
                }

            } else if ( onEdge ) {
#ifdef DEBUG_RP
                printRaypathData(curr_pt, g, onNode, onEdge, onFace, cellNo,
                                 nodeNo, edgeNodes, faceNodes);
#endif
                // find cells common to edge
                std::vector<T2> cells;
                for ( auto nc0=nodes[edgeNodes[0]].getOwners().begin(); nc0!=nodes[edgeNodes[0]].getOwners().end(); ++nc0 ) {
                    if ( std::find(nodes[edgeNodes[1]].getOwners().begin(), nodes[edgeNodes[1]].getOwners().end(), *nc0)!=nodes[edgeNodes[1]].getOwners().end() ) {
                        cells.push_back( *nc0 );
                    }
                }
                if ( rp_method < 2 ) {
                    std::set<NODE*> nnodes;
                    for (size_t n=0; n<cells.size(); ++n ) {
                        getNeighborNodes(cells[n], nnodes);
                    }
                    T1 d01 = nodes[edgeNodes[0]].getDistance(nodes[edgeNodes[1]]);
                    T1 w0 = curr_pt.getDistance(nodes[edgeNodes[1]]) / d01;
                    T1 w1 = curr_pt.getDistance(nodes[edgeNodes[0]]) / d01;
                    T1 curr_t = nodes[edgeNodes[0]].getTT(threadNo)*w0 + nodes[edgeNodes[1]].getTT(threadNo)*w1;
                    g = grad3d->compute(curr_pt, curr_t, nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(2);
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    ref_pt[0] = &(nodes[edgeNodes[0]]);
                    ref_pt[1] = &(nodes[edgeNodes[1]]);
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }
                checkCloseToTx(curr_pt, g, edgeNodes, Tx, txCell);

                bool foundIntersection=false;
                for (size_t n=0; n<cells.size(); ++n ) {

                    T2 testCellNo = cells[n];

                    // there are 2 faces that might be intersected
                    std::array<T2,2> edgeNodes2;
                    size_t n2=0;
                    for ( auto nn=this->neighbors[testCellNo].begin(); nn!= this->neighbors[testCellNo].end(); ++nn ) {
                        if ( *nn!=edgeNodes[0] && *nn!=edgeNodes[1] && nodes[*nn].isPrimary() ) {
                            edgeNodes2[n2++] = *nn;
                        }
                    }

                    sxyz<T1> pt_i;
                    T2 itmpNode;
                    foundIntersection = intersectVecTriangle(curr_pt, g,
                                                             edgeNodes[0],
                                                             edgeNodes2[0],
                                                             edgeNodes2[1], pt_i);
                    itmpNode = edgeNodes[0];
                    if ( !foundIntersection ) {
                        foundIntersection = intersectVecTriangle(curr_pt, g,
                                                                 edgeNodes[1],
                                                                 edgeNodes2[0],
                                                                 edgeNodes2[1], pt_i);
                        itmpNode = edgeNodes[1];
                    }
                    if ( !foundIntersection ) {
                        continue;
                    }

                    cellNo = testCellNo;
                    curr_pt = pt_i;

                    bool break_flag = check_pt_location(curr_pt, this->neighbors[cellNo],
                                                        {itmpNode, edgeNodes2[0], edgeNodes2[1]},
                                                        onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);
#ifdef DEBUG_RP
                    r_data[0].push_back(curr_pt);
#endif

                    tt += slowness[cellNo] * prev_pt.getDistance( curr_pt );
                    prev_pt = curr_pt;

                    if ( break_flag ) break;

                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        tt = 0.0;
                        reachedTx = true;
                    }
                    break;
                }
                if ( foundIntersection == false ) {
                    // we must be going outside the mesh
                    // hack: project gradient on face and find intersection
#ifdef DEBUG_RP
                    std::cout << "\n\nWarning: raypath (onEdge) likely going outside mesh for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << '\n';
                    std::cout << "         Projecting gradient on external face and resuming" << std::endl;
#endif
                    sxyz<T1> pt_i;
                    std::array<T2,2> pen = edgeNodes;
                    g = projectOnFace(curr_pt, g, edgeNodes, cells, pt_i);
                    if ( g.x==0.0 && g.y==0.0 && g.z==0.0 ) {
                        foundIntersection = false;
                    } else {
                        foundIntersection = true;
                    }

                    if ( foundIntersection == false || curr_pt == pt_i ) {
                        std::cout << "\n\nWarning: finding raypath (onEdge) failed to converge for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        tt = 0.0;
                        reachedTx = true;
                        break;
                    }

                    // we might be on one of the nodes
                    if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[0];
                        pt_i = nodes[edgeNodes[0]];
                        onEdge = false;
                    } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[1];
                        pt_i = nodes[edgeNodes[1]];
                        onEdge = false;
                    }

                    T1 dt;
                    // find traversed cell
                    if ( onNode ) {
                        T2 traversedCell = 0;
                        for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                            if ( std::find( nodes[pen[0]].getOwners().begin(), nodes[pen[0]].getOwners().end(), *nc ) != nodes[pen[0]].getOwners().end() &&
                                std::find( nodes[pen[1]].getOwners().begin(), nodes[pen[1]].getOwners().end(), *nc ) != nodes[pen[1]].getOwners().end() ) {
                                traversedCell = *nc;
                                break;
                            }
                        }
                        dt = slowness[traversedCell] * prev_pt.getDistance( curr_pt );
                    } else {
                        T2 traversedCell = 0;
                        std::array<T2,3> fn;
                        fn[0] = pen[0];
                        fn[1] = pen[1];
                        if ( edgeNodes[0] != fn[0] && edgeNodes[0] != fn[1]) {
                            fn[2] = edgeNodes[0];
                        } else {
                            fn[2] = edgeNodes[1];
                        }
                        for ( auto nc=nodes[fn[0]].getOwners().begin(); nc!=nodes[fn[0]].getOwners().end(); ++nc ) {
                            if ( std::find( nodes[fn[1]].getOwners().begin(), nodes[fn[1]].getOwners().end(), *nc ) != nodes[fn[1]].getOwners().end() &&
                                std::find( nodes[fn[2]].getOwners().begin(), nodes[fn[2]].getOwners().end(), *nc ) != nodes[fn[2]].getOwners().end() ) {
                                traversedCell = *nc;
                                break;
                            }
                        }
                        dt = slowness[traversedCell] * prev_pt.getDistance( curr_pt );
                    }

                    curr_pt = pt_i;
#ifdef DEBUG_RP
                    r_data[0].push_back(curr_pt);
#endif
                    tt += dt;
                    prev_pt = curr_pt;
                }

            } else if ( onFace ) {
#ifdef DEBUG_RP
                printRaypathData(curr_pt, g, onNode, onEdge, onFace, cellNo,
                                 nodeNo, edgeNodes, faceNodes);
#endif
                std::array<T2,4> itmp = getPrimary(cellNo);
                if ( rp_method < 2 ) {
                    std::set<NODE*> nnodes;
                    getNeighborNodes(cellNo, nnodes);
                    T1 curr_t;
                    if ( atRx ) {
                        curr_t = Interpolator<T1>::trilinearTime(curr_pt,
                                                                 nodes[itmp[0]],
                                                                 nodes[itmp[1]],
                                                                 nodes[itmp[2]],
                                                                 nodes[itmp[3]],
                                                                 threadNo);
                        atRx = false;
                    } else {
                        curr_t = Interpolator<T1>::bilinearTime(curr_pt,
                                                                nodes[faceNodes[0]],
                                                                nodes[faceNodes[1]],
                                                                nodes[faceNodes[2]],
                                                                threadNo);
                    }
                    g = grad3d->compute(curr_pt, curr_t, nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(3);
                    if ( atRx ) {
                        ref_pt[0] = &(nodes[itmp[0]]);
                        ref_pt[1] = &(nodes[itmp[1]]);
                        ref_pt[2] = &(nodes[itmp[2]]);
                        ref_pt.push_back( &(nodes[itmp[3]]) );
                        atRx = false;
                    } else {
                        ref_pt[0] = &(nodes[faceNodes[0]]);
                        ref_pt[1] = &(nodes[faceNodes[1]]);
                        ref_pt[2] = &(nodes[faceNodes[2]]);
                    }
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }
                checkCloseToTx(curr_pt, g, cellNo, Tx, txCell);

                std::array<T2,3> ind[4] = {
                    {itmp[0], itmp[1], itmp[2]},
                    {itmp[0], itmp[1], itmp[3]},
                    {itmp[0], itmp[2], itmp[3]},
                    {itmp[1], itmp[2], itmp[3]}};
                for ( size_t n=0; n<4; ++n ) {
                    std::sort( ind[n].begin(), ind[n].end() );
                }
                // there are 3 faces that might be intersected

                bool foundIntersection = false;
                for ( size_t n=0; n<4; ++n ) {
                    if ( ind[n] == faceNodes ) continue;

                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                             ind[n][1], ind[n][2],
                                                             pt_i);

                    if ( !foundIntersection )
                        continue;

                    curr_pt = pt_i;

                    bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);
#ifdef DEBUG_RP
                    r_data[0].push_back(curr_pt);
#endif

                    tt += slowness[cellNo] * prev_pt.getDistance( curr_pt );
                    prev_pt = curr_pt;

                    if ( break_flag ) break;

                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        tt = 0.0;
                        reachedTx = true;
                    }
                    break;
                }

                if ( foundIntersection == false ) {

                    if ( (faceNodes[0] == faceNodes[1]) && (faceNodes[0] == faceNodes[2]) ) {
                        // we have started on a face and cellNo was incorrectly picked
                        // find adjacent cell
                        cellNo = findAdjacentCell2(faceNodesStart, cellNo);
                    } else {
                        // we must be on an face with gradient pointing slightly outward tetrahedron
                        // return in other cell but keep gradient
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                    }

                    std::array<T2,4> itmp = getPrimary(cellNo);
                    ind[0] = {itmp[0], itmp[1], itmp[2]};
                    ind[1] = {itmp[0], itmp[1], itmp[3]};
                    ind[2] = {itmp[0], itmp[2], itmp[3]};
                    ind[3] = {itmp[1], itmp[2], itmp[3]};

                    for ( size_t n=0; n<4; ++n ) {
                        std::sort( ind[n].begin(), ind[n].end() );
                    }

                    for ( size_t n=0; n<4; ++n ) {
                        if ( ind[n] == faceNodes ) continue;

                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                 ind[n][1], ind[n][2],
                                                                 pt_i);

                        if ( !foundIntersection ) {
                            continue;
                        }

                        curr_pt = pt_i;

                        bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                            nodeNo, onEdge, edgeNodes,
                                                            onFace, faceNodes);
#ifdef DEBUG_RP
                        r_data[0].push_back(curr_pt);
#endif

                        tt += slowness[cellNo] * prev_pt.getDistance( curr_pt );
                        prev_pt = curr_pt;

                        if ( break_flag ) break;

                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                            << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                            tt = 0.0;
                            reachedTx = true;
                        }
                        break;
                    }
                }
                if ( foundIntersection == false ) {
                    // we must be going outside the mesh
                    // hack: project gradient on face and continue
#ifdef DEBUG_RP
                    std::cout << "\n\nWarning: raypath (onFace) likely going outside mesh for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << '\n';
                    std::cout << "         Projecting gradient on external face and resuming" << std::endl;
#endif
                    g = projectOnFace(g, faceNodes);

                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecEdge(curr_pt, g, faceNodes, pt_i, edgeNodes);

                    if ( foundIntersection == false ) {
                        std::cout << "\n\nWarning: finding raypath (onFace) failed to converge for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        tt = 0.0;
                        reachedTx = true;
                        break;
                    }

                    // we might be on one of the nodes
                    if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[0];
                        pt_i = nodes[edgeNodes[0]];
                        onEdge = false;
                        onFace = false;
                    } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[1];
                        pt_i = nodes[edgeNodes[1]];
                        onEdge = false;
                        onFace = false;
                    } else {
                        onEdge = true;
                        onFace = false;
                    }
                    curr_pt = pt_i;
#ifdef DEBUG_RP
                    r_data[0].push_back(curr_pt);
#endif
                    T2 traversedCell = 0;
                    for ( auto nc=nodes[faceNodes[0]].getOwners().begin(); nc!=nodes[faceNodes[0]].getOwners().end(); ++nc ) {
                        if ( std::find( nodes[faceNodes[1]].getOwners().begin(), nodes[faceNodes[1]].getOwners().end(), *nc ) != nodes[faceNodes[1]].getOwners().end() &&
                            std::find( nodes[faceNodes[2]].getOwners().begin(), nodes[faceNodes[2]].getOwners().end(), *nc ) != nodes[faceNodes[2]].getOwners().end() ) {
                            traversedCell = *nc;
                            break;
                        }
                    }

                    tt += slowness[traversedCell] * prev_pt.getDistance( curr_pt );
                    prev_pt = curr_pt;
                }
            } else { // at Rx, somewhere in a tetrahedron
#ifdef DEBUG_RP
                printRaypathData(curr_pt, g, onNode, onEdge, onFace, cellNo,
                                 nodeNo, edgeNodes, faceNodes);
#endif
                std::array<T2,4> itmp = getPrimary(cellNo);
                if ( rp_method < 2 ) {
                    std::set<NODE*> nnodes;
                    getNeighborNodes(cellNo, nnodes);
                    T1 curr_t = Interpolator<T1>::trilinearTime(curr_pt,
                                                                nodes[itmp[0]],
                                                                nodes[itmp[1]],
                                                                nodes[itmp[2]],
                                                                nodes[itmp[3]],
                                                                threadNo);
                    g = grad3d->compute(curr_pt, curr_t, nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(4);
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    ref_pt[0] = &(nodes[itmp[0]]);
                    ref_pt[1] = &(nodes[itmp[1]]);
                    ref_pt[2] = &(nodes[itmp[2]]);
                    ref_pt[3] = &(nodes[itmp[3]]);
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }
                checkCloseToTx(curr_pt, g, cellNo, Tx, txCell);

                std::array<T2,3> ind[4] = {
                    {itmp[0], itmp[1], itmp[2]},
                    {itmp[0], itmp[1], itmp[3]},
                    {itmp[0], itmp[2], itmp[3]},
                    {itmp[1], itmp[2], itmp[3]}};
                for ( size_t n=0; n<4; ++n ) {
                    std::sort( ind[n].begin(), ind[n].end() );
                }

                bool foundIntersection = false;
                for ( size_t n=0; n<4; ++n ) {

                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                             ind[n][1], ind[n][2],
                                                             pt_i);

                    if ( !foundIntersection )
                        continue;

                    curr_pt = pt_i;

                    bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);
#ifdef DEBUG_RP
                    r_data[0].push_back(curr_pt);
#endif

                    tt += slowness[cellNo] * prev_pt.getDistance( curr_pt );
                    prev_pt = curr_pt;

                    if ( break_flag ) break;

                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        tt = 0.0;
                        reachedTx = true;
                    }
                    break;
                }
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath within cell failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    tt = 0.0;
                    reachedTx = true;
                }
            }

            if ( onNode ) {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        if ( curr_pt.getDistance( Tx[nt] ) < minDist ) {
                            tt += t0[nt];
                            reachedTx = true;
                            break;
                        }
                    } else if ( txOnEdge[nt] ) {
                        if ( curr_pt.getDistance(nodes[txEdges[nt][0]]) < minDist ||
                            curr_pt.getDistance(nodes[txEdges[nt][1]]) < minDist ) {

                            tt += t0[nt] + slowness[txCell[nt]] * prev_pt.getDistance( Tx[nt] );
                            reachedTx = true;
#ifdef DEBUG_RP
                            r_data[0].push_back(Tx[nt]);
#endif
                            break;
                        }
                    } else if ( txOnFace[nt] ) {
                        if ( curr_pt.getDistance(nodes[txFaces[nt][0]]) < minDist ||
                            curr_pt.getDistance(nodes[txFaces[nt][1]]) < minDist ||
                            curr_pt.getDistance(nodes[txFaces[nt][2]]) < minDist ) {

                            tt += t0[nt] + slowness[txCell[nt]] * prev_pt.getDistance( Tx[nt] );
                            reachedTx = true;
#ifdef DEBUG_RP
                            r_data[0].push_back(Tx[nt]);
#endif
                            break;
                        }
                    }
                }
            } if ( onEdge ) {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        if ( txNode[nt] == edgeNodes[0] || txNode[nt] == edgeNodes[1] ) {
                            tt += t0[nt] + slowness[txCell[nt]] * prev_pt.getDistance( Tx[nt] );
                            reachedTx = true;
#ifdef DEBUG_RP
                            r_data[0].push_back(Tx[nt]);
#endif
                            break;
                        }
                    } else {
                        std::array<T2,4> itmp = getPrimary(txCell[nt]);
                        // find adjacent cells
                        const T2 ind[6][2] = {
                            {itmp[0], itmp[1]},
                            {itmp[0], itmp[2]},
                            {itmp[0], itmp[3]},
                            {itmp[1], itmp[2]},
                            {itmp[1], itmp[3]},
                            {itmp[2], itmp[3]} };
                        for ( size_t ne=0; ne<6; ++ne ) {
                            if ( (ind[ne][0] == edgeNodes[0] && ind[ne][1] == edgeNodes[1]) ||
                                (ind[ne][0] == edgeNodes[1] && ind[ne][1] == edgeNodes[0]) ) {

                                tt += t0[nt] + slowness[txCell[nt]] * prev_pt.getDistance( Tx[nt] );
                                reachedTx = true;
#ifdef DEBUG_RP
                                r_data[0].push_back(Tx[nt]);
#endif
                                break;
                            }
                        }
                    }
                }
            } else {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        for ( auto nc=nodes[txNode[nt]].getOwners().begin(); nc!=nodes[txNode[nt]].getOwners().end(); ++nc ) {
                            if ( cellNo == *nc ) {
                                tt += t0[nt] + slowness[cellNo] * prev_pt.getDistance( Tx[nt] );
                                reachedTx = true;
#ifdef DEBUG_RP
                                r_data[0].push_back(Tx[nt]);
#endif
                                break;
                            }
                        }
                    } else {
                        if ( cellNo == txCell[nt] ) {
                            tt += t0[nt] + slowness[cellNo] * prev_pt.getDistance( Tx[nt] );
#ifdef DEBUG_RP
                            r_data[0].push_back(Tx[nt]);
#endif
                            reachedTx = true;
                        } else {
                            for ( size_t nn=0; nn<txNeighborCells[nt].size(); ++nn ) {
                                if ( cellNo == txNeighborCells[nt][nn] ) {
                                    std::array<T2,4> itmp = getPrimary(txCell[nt]);
                                    std::array<T2,3> ind[4] = {
                                        { { itmp[0], itmp[1], itmp[2] } },
                                        { { itmp[0], itmp[1], itmp[3] } },
                                        { { itmp[0], itmp[2], itmp[3] } },
                                        { { itmp[1], itmp[2], itmp[3] } }
                                    };

                                    bool found = false;
                                    for ( size_t n=0; n<4; ++n ) {
                                        std::sort( ind[n].begin(), ind[n].end() );
                                        if ( faceNodes == ind[n] ) {
                                            found = true;
                                            break;
                                        }
                                    }
                                    if ( found ) {
                                        tt += t0[nt] + slowness[cellNo] * prev_pt.getDistance( Tx[nt] );
#ifdef DEBUG_RP
                                        r_data[0].push_back(Tx[nt]);
#endif
                                        reachedTx = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if ( reachedTx ) break;
                }
            }
        }
#ifdef DEBUG_RP
        std::ostringstream fname;
        fname << "raypath_" << Rx.x << '_' << Rx.y << '_' << Rx.z << ".vtp";
        saveRayPaths(fname.str(), r_data);
#endif
        delete grad3d;
        return tt;
    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::printRaypathData(const sxyz<T1>& curr_pt,
                                                const sxyz<T1>& g,
                                                const bool onNode,
                                                const bool onEdge,
                                                const bool onFace,
                                                const T2 cellNo,
                                                const T2 nodeNo,
                                                const std::array<T2,2> &edgeNodes,
                                                const std::array<T2,3> &faceNodes) const {
        std::array<T2,4> itmp = getPrimary(cellNo);
        std::cout << "\n*** RP debug data\n   curr_pt: " << curr_pt << '\n'
        << "         g: " << g << '\n'
        << "    cellNo: " << cellNo << '\n'
        << "  vertices: " << nodes[itmp[0]] << '\n'
        << "          : " << nodes[itmp[1]] << '\n'
        << "          : " << nodes[itmp[2]] << '\n'
        << "          : " << nodes[itmp[3]] << '\n';
        if ( onNode ) {
            std::cout << "\tonNode\n"
            << "\t    nodeNo: " << nodeNo << '\n'
            << "\t    coords: " << nodes[nodeNo] << '\n';
        }
        if ( onEdge ) {
            std::cout << "\tonEdge\n"
            << "\t    edgeNo: " << edgeNodes[0] << ' ' << edgeNodes[1] << '\n'
            << "\t    coords: " << nodes[edgeNodes[0]] << '\n'
            << "\t    coords: " << nodes[edgeNodes[1]] << '\n';
        }
        if ( onFace ) {
            std::cout << "\tonFace\n"
            << "\t    faceNo: " << faceNodes[0] << ' ' << faceNodes[1] << ' ' << faceNodes[2] << '\n'
            << "\t    coords: " << nodes[faceNodes[0]] << '\n'
            << "\t    coords: " << nodes[faceNodes[1]] << '\n'
            << "\t    coords: " << nodes[faceNodes[2]] << '\n';
        }
        std::cout << std::endl;
    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::getRaypath(const std::vector<sxyz<T1>>& Tx,
                                          const sxyz<T1> &Rx,
                                          std::vector<sxyz<T1>> &r_data,
                                          const size_t threadNo) const {

        T1 minDist = small;
        r_data.emplace_back( Rx );

        for ( size_t ns=0; ns<Tx.size(); ++ns ) {
            if ( Rx == Tx[ns] ) {
                return;
            }
        }

        std::vector<bool> txOnNode( Tx.size(), false );
        std::vector<bool> txOnEdge( Tx.size(), false );
        std::vector<bool> txOnFace( Tx.size(), false );
        std::vector<T2> txNode( Tx.size() );
        std::vector<T2> txCell( Tx.size() );
        std::vector<std::array<T2,2>> txEdges( Tx.size() );
        std::vector<std::array<T2,3>> txFaces( Tx.size() );
        std::vector<std::vector<T2>> txNeighborCells( Tx.size() );
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            for ( T2 nn=0; nn<nodes.size(); ++nn ) {
                if ( nodes[nn].isPrimary() ) {
                    if ( nodes[nn] == Tx[nt] ) {
                        txOnNode[nt] = true;
                        txNode[nt] = nn;
                        break;
                    }
                }
            }
        }
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            if ( !txOnNode[nt] ) {
                txCell[nt] = getCellNo( Tx[nt] );

                std::array<T2,4> itmp = getPrimary(txCell[nt]);
                // find adjacent cells
                T2 ind[6][2] = {
                    {itmp[0], itmp[1]},
                    {itmp[0], itmp[2]},
                    {itmp[0], itmp[3]},
                    {itmp[1], itmp[2]},
                    {itmp[1], itmp[3]},
                    {itmp[2], itmp[3]} };

                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    for ( auto nc0=nodes[ind[nedge][0]].getOwners().begin(); nc0!=nodes[ind[nedge][0]].getOwners().end(); ++nc0 ) {
                        if ( std::find(nodes[ind[nedge][1]].getOwners().begin(), nodes[ind[nedge][1]].getOwners().end(), *nc0)!=nodes[ind[nedge][1]].getOwners().end() )
                            txNeighborCells[nt].push_back( *nc0 );
                    }
                }
                // check if on edge
                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    if ( distSqPointToSegment( &nodes[ind[nedge][0]], &nodes[ind[nedge][1]], Tx[nt]) < small2 ) {
                        txOnEdge[nt] = true;
                        txEdges[nt][0] = ind[nedge][0];
                        txEdges[nt][1] = ind[nedge][1];
                        break;
                    }
                }
                if ( !txOnEdge[nt] ) {
                    // check if on face
                    const T2 indf[4][3] = {
                        {itmp[0], itmp[1], itmp[2]},
                        {itmp[0], itmp[1], itmp[3]},
                        {itmp[0], itmp[2], itmp[3]},
                        {itmp[1], itmp[2], itmp[3]} };
                    for ( size_t nface=0; nface<4; ++nface ) {
                        if ( testInTriangle(&nodes[indf[nface][0]], &nodes[indf[nface][1]], &nodes[indf[nface][2]], Tx[nt]) ) {
                            txOnFace[nt] = true;
                            txFaces[nt][0] = indf[nface][0];
                            txFaces[nt][1] = indf[nface][1];
                            txFaces[nt][2] = indf[nface][2];
                            break;
                        }
                    }
                }
            }
        }

        T2 cellNo, nodeNo;
        sxyz<T1> curr_pt( Rx );

        bool onNode = false;
        bool onEdge = false;
        bool onFace = false;
        std::array<T2,2> edgeNodes{ {0, 0} };
        std::array<T2,3> faceNodes{ {0, 0, 0} };
        std::array<T2,3> faceNodesStart{ {0, 0, 0} };

        Grad3D<T1,NODE>* grad3d = nullptr;
        if ( rp_method == 0 ) {
            grad3d = new Grad3D_ls_fo<T1,NODE>();
        } else if ( rp_method == 1 ) {
            grad3d = new Grad3D_ls_so<T1,NODE>();
        } else if ( rp_method == 2 ) {
            grad3d = new Grad3D_ab<T1,NODE>();
        }
        bool reachedTx = false;

        for ( T2 nn=0; nn<nodes.size(); ++nn ) {
            if ( nodes[nn].getDistance( curr_pt ) < small ) {
                nodeNo = nn;
                onNode = true;
                break;
            }
        }
        if ( !onNode ) {
            cellNo = getCellNo( curr_pt );

            std::array<T2,4> itmp = getPrimary(cellNo);
            T2 ind[6][2] = {
                {itmp[0], itmp[1]},
                {itmp[0], itmp[2]},
                {itmp[0], itmp[3]},
                {itmp[1], itmp[2]},
                {itmp[1], itmp[3]},
                {itmp[2], itmp[3]} };

            for ( size_t n=0; n<6; ++n ) {
                if ( areCollinear(curr_pt, nodes[ind[n][0]], nodes[ind[n][1]]) ) {
                    onEdge = true;
                    edgeNodes[0] = ind[n][0];
                    edgeNodes[1] = ind[n][1];
                    break;
                }
            }
        }
        if ( !onNode && !onEdge ) {
            std::array<T2,4> itmp = getPrimary(cellNo);
            std::array<T2,3> ind[4] = {
                {itmp[0], itmp[1], itmp[2]},
                {itmp[0], itmp[1], itmp[3]},
                {itmp[0], itmp[2], itmp[3]},
                {itmp[1], itmp[2], itmp[3]}};
            for ( size_t n=0; n<4; ++n ) {
                std::sort( ind[n].begin(), ind[n].end() );
            }

            for ( size_t n=0; n<4; ++n ) {
                if ( areCoplanar(curr_pt, nodes[ind[n][0]], nodes[ind[n][1]], nodes[ind[n][2]]) ) {
                    onFace = true;
                    faceNodesStart = ind[n];
                    break;
                }
            }
        }

        for ( auto nt=0; nt<txCell.size(); ++nt ) {
            if ( cellNo == txCell[nt] ) {
                r_data.push_back( Tx[nt] );
                reachedTx = true;
                break;
            }
        }

        sxyz<T1> g;
        while ( reachedTx == false ) {

            if ( onNode ) {

                if ( rp_method < 2 ) {
                    // find cells common to edge
                    std::set<NODE*> nnodes;
                    for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                        getNeighborNodes(*nc, nnodes);
                    }
                    // compute gradient with nodes from all common cells
                    g = grad3d->compute(curr_pt, nodes[nodeNo].getTT(threadNo), nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(1);
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    ref_pt[0] = &(nodes[nodeNo]);
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }

                // find cell for which gradient intersect opposing face
                bool foundIntersection = false;
                for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {

                    std::array<T2,3> nb;
                    size_t n=0;
                    for (auto nn=this->neighbors[*nc].begin(); nn!=this->neighbors[*nc].end(); ++nn ) {
                        if ( *nn != nodeNo && nodes[*nn].isPrimary() ) {
                            nb[n++] = *nn;
                        }
                    }
                    std::sort(nb.begin(), nb.end());

                    foundIntersection = intersectVecTriangle( nodeNo, g, nb[0], nb[1], nb[2], curr_pt);
                    if ( !foundIntersection ) {
                        continue;
                    }

                    //  check if cell is (one of) TxCell(s)
                    for (size_t nt=0; nt<Tx.size(); ++nt) {
                        if ( *nc == txCell[nt] ) {
                            std::array<T2,4> itmp = getPrimary(*nc);

                            r_data.push_back( Tx[nt] );
                            reachedTx = true;
                            break;
                        }
                    }
                    if ( reachedTx ) {
                        break;
                    }

                    bool break_flag = check_pt_location(curr_pt, nb, onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);

                    r_data.push_back( curr_pt );

                    if ( break_flag ) break;

                    // find next cell
                    cellNo = findAdjacentCell1(faceNodes, nodeNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }

                if ( foundIntersection == false ) {
                    // projet gradient on face and find intersection on next edge
                    sxyz<T1> pt_i;
                    foundIntersection = projectOnFace(curr_pt, nodeNo, g, edgeNodes, pt_i);

                    if ( foundIntersection == false ) {
                        std::cout << "\n\nWarning: finding raypath on node failed to converge for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                        break;
                    }

                    // find which cell we are in
                    for ( size_t nc=0; nc<tetrahedra.size(); ++nc ) {
                        std::array<T2,4> itet = {tetrahedra[nc].i[0],
                            tetrahedra[nc].i[1],
                            tetrahedra[nc].i[2],
                            tetrahedra[nc].i[3]};
                        // because we are at the surface, there is only one cell with nodeNo & edgeNodes
                        if (std::find(itet.begin(), itet.end(), nodeNo) != itet.end() &&
                            std::find(itet.begin(), itet.end(), edgeNodes[0]) != itet.end() &&
                            std::find(itet.begin(), itet.end(), edgeNodes[1]) != itet.end()) {
                            cellNo = static_cast<T2>(nc);
                            break;
                        }
                    }

                    //  check if cell is (one of) TxCell(s)
                    for (size_t nt=0; nt<Tx.size(); ++nt) {
                        if ( cellNo == txCell[nt] ) {
                            std::array<T2,4> itmp = getPrimary(cellNo);

                            r_data.push_back( Tx[nt] );
                            reachedTx = true;
                            break;
                        }
                    }
                    if ( reachedTx ) {
                        break;
                    }
                    // we might be on one of the nodes
                    if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[0];
                        pt_i = nodes[edgeNodes[0]];
                        onEdge = false;
                    } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[1];
                        pt_i = nodes[edgeNodes[1]];
                        onEdge = false;
                    } else {
                        onEdge = true;
                        onNode = false;
                    }

                    curr_pt = pt_i;
                    r_data.push_back( curr_pt );
                }

            } else if ( onEdge ) {

                // find cells common to edge
                std::vector<T2> cells;
                for ( auto nc0=nodes[edgeNodes[0]].getOwners().begin(); nc0!=nodes[edgeNodes[0]].getOwners().end(); ++nc0 ) {
                    if ( std::find(nodes[edgeNodes[1]].getOwners().begin(), nodes[edgeNodes[1]].getOwners().end(), *nc0)!=nodes[edgeNodes[1]].getOwners().end() ) {
                        cells.push_back( *nc0 );
                    }
                }
                if ( rp_method < 2 ) {
                    std::set<NODE*> nnodes;
                    for (size_t n=0; n<cells.size(); ++n ) {
                        getNeighborNodes(cells[n], nnodes);
                    }
                    T1 d01 = nodes[edgeNodes[0]].getDistance(nodes[edgeNodes[1]]);
                    T1 w0 = curr_pt.getDistance(nodes[edgeNodes[1]]) / d01;
                    T1 w1 = curr_pt.getDistance(nodes[edgeNodes[0]]) / d01;
                    T1 curr_t = nodes[edgeNodes[0]].getTT(threadNo)*w0 + nodes[edgeNodes[1]].getTT(threadNo)*w1;
                    g = grad3d->compute(curr_pt, curr_t, nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(2);
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    ref_pt[0] = &(nodes[edgeNodes[0]]);
                    ref_pt[1] = &(nodes[edgeNodes[1]]);
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }
                checkCloseToTx(curr_pt, g, edgeNodes, Tx, txCell);

                bool foundIntersection=false;
                for (size_t n=0; n<cells.size(); ++n ) {

                    T2 testCellNo = cells[n];

                    // there are 2 faces that might be intersected
                    std::array<T2,2> edgeNodes2;
                    size_t n2=0;
                    for ( auto nn=this->neighbors[testCellNo].begin(); nn!= this->neighbors[testCellNo].end(); ++nn ) {
                        if ( *nn!=edgeNodes[0] && *nn!=edgeNodes[1] && nodes[*nn].isPrimary() ) {
                            edgeNodes2[n2++] = *nn;
                        }
                    }

                    sxyz<T1> pt_i;
                    T2 itmpNode;
                    foundIntersection = intersectVecTriangle(curr_pt, g,
                                                             edgeNodes[0],
                                                             edgeNodes2[0],
                                                             edgeNodes2[1], pt_i);
                    itmpNode = edgeNodes[0];
                    if ( !foundIntersection ) {
                        foundIntersection = intersectVecTriangle(curr_pt, g,
                                                                 edgeNodes[1],
                                                                 edgeNodes2[0],
                                                                 edgeNodes2[1], pt_i);
                        itmpNode = edgeNodes[1];
                    }
                    if ( !foundIntersection ) {
                        continue;
                    }

                    cellNo = testCellNo;
                    curr_pt = pt_i;

                    bool break_flag = check_pt_location(curr_pt, this->neighbors[cellNo],
                                                        {itmpNode, edgeNodes2[0], edgeNodes2[1]},
                                                        onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);

                    r_data.push_back( curr_pt );

                    if ( break_flag ) break;

                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                if ( foundIntersection == false ) {
                    sxyz<T1> pt_i;
                    g = projectOnFace(curr_pt, g, edgeNodes, cells, pt_i);
                    if ( g.x==0.0 && g.y==0.0 && g.z==0.0 ) {
                        foundIntersection = false;
                    } else {
                        foundIntersection = true;
                    }

                    if ( foundIntersection == false || curr_pt == pt_i ) {
                        std::cout << "\n\nWarning: finding raypath on edge failed to converge for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                    }

                    // we might be on one of the nodes
                    if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[0];
                        pt_i = nodes[edgeNodes[0]];
                        onEdge = false;
                    } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[1];
                        pt_i = nodes[edgeNodes[1]];
                        onEdge = false;
                    }

                    curr_pt = pt_i;
                    r_data.push_back(curr_pt);
                }

            } else if ( onFace ) {

                std::array<T2,4> itmp = getPrimary(cellNo);
                if ( rp_method < 2 ) {
                    std::set<NODE*> nnodes;
                    getNeighborNodes(cellNo, nnodes);
                    T1 curr_t;
                    if ( r_data.size() <= 1 ) {
                        curr_t = Interpolator<T1>::trilinearTime(curr_pt,
                                                                 nodes[itmp[0]],
                                                                 nodes[itmp[1]],
                                                                 nodes[itmp[2]],
                                                                 nodes[itmp[3]],
                                                                 threadNo);
                    } else {
                        curr_t = Interpolator<T1>::bilinearTime(curr_pt,
                                                                nodes[faceNodes[0]],
                                                                nodes[faceNodes[1]],
                                                                nodes[faceNodes[2]],
                                                                threadNo);
                    }
                    g = grad3d->compute(curr_pt, curr_t, nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(3);
                    if ( r_data.size() <= 1 ) {
                        ref_pt[0] = &(nodes[itmp[0]]);
                        ref_pt[1] = &(nodes[itmp[1]]);
                        ref_pt[2] = &(nodes[itmp[2]]);
                        ref_pt.push_back( &(nodes[itmp[3]]) );
                    } else {
                        ref_pt[0] = &(nodes[faceNodes[0]]);
                        ref_pt[1] = &(nodes[faceNodes[1]]);
                        ref_pt[2] = &(nodes[faceNodes[2]]);
                    }
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }
                checkCloseToTx(curr_pt, g, cellNo, Tx, txCell);

                std::array<T2,3> ind[4] = {
                    {itmp[0], itmp[1], itmp[2]},
                    {itmp[0], itmp[1], itmp[3]},
                    {itmp[0], itmp[2], itmp[3]},
                    {itmp[1], itmp[2], itmp[3]}
                };
                for ( size_t n=0; n<4; ++n ) {
                    std::sort( ind[n].begin(), ind[n].end() );
                }
                // there are 3 faces that might be intersected

                bool foundIntersection = false;
                for ( size_t n=0; n<4; ++n ) {
                    if ( ind[n] == faceNodes ) continue;

                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                             ind[n][1], ind[n][2],
                                                             pt_i);

                    if ( !foundIntersection )
                        continue;

                    curr_pt = pt_i;

                    bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);

                    r_data.push_back( curr_pt );

                    if ( break_flag ) break;

                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }

                if ( foundIntersection == false ) {

                    if ( (faceNodes[0] == faceNodes[1]) && (faceNodes[0] == faceNodes[2]) ) {
                        // we have started on a face and cellNo was incorrectly picked
                        // find adjacent cell
                        cellNo = findAdjacentCell2(faceNodesStart, cellNo);
                    } else {
                        // we must be on an face with gradient pointing slightly outward tetrahedron
                        // return in other cell but keep gradient
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                    }

                    std::array<T2,4> itmp = getPrimary(cellNo);
                    ind[0] = {itmp[0], itmp[1], itmp[2]};
                    ind[1] = {itmp[0], itmp[1], itmp[3]};
                    ind[2] = {itmp[0], itmp[2], itmp[3]};
                    ind[3] = {itmp[1], itmp[2], itmp[3]};

                    for ( size_t n=0; n<4; ++n ) {
                        std::sort( ind[n].begin(), ind[n].end() );
                    }

                    for ( size_t n=0; n<4; ++n ) {
                        if ( ind[n] == faceNodes ) continue;

                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                 ind[n][1], ind[n][2],
                                                                 pt_i);

                        if ( !foundIntersection ) {
                            continue;
                        }
                        curr_pt = pt_i;

                        bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                            nodeNo, onEdge, edgeNodes,
                                                            onFace, faceNodes);

                        r_data.push_back( curr_pt );

                        if ( break_flag ) break;

                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                            << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                            r_data.resize(1);
                            r_data[0] = Rx;
                            reachedTx = true;
                        }
                        break;
                    }
                }
                if ( foundIntersection == false ) {
                    g = projectOnFace(g, faceNodes);

                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecEdge(curr_pt, g, faceNodes, pt_i, edgeNodes);

                    if ( foundIntersection == false ) {
                        std::cout << "\n\nWarning: finding raypath on face failed to converge for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                        break;
                    }

                    // we might be on one of the nodes
                    if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[0];
                        pt_i = nodes[edgeNodes[0]];
                        onEdge = false;
                        onFace = false;
                    } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[1];
                        pt_i = nodes[edgeNodes[1]];
                        onEdge = false;
                        onFace = false;
                    } else {
                        onEdge = true;
                        onFace = false;
                    }
                    curr_pt = pt_i;
                    r_data.push_back( curr_pt );
                }
            } else { // at Rx, somewhere in a tetrahedron

                std::array<T2,4> itmp = getPrimary(cellNo);
                if ( rp_method < 2 ) {
                    std::set<NODE*> nnodes;
                    getNeighborNodes(cellNo, nnodes);
                    T1 curr_t = Interpolator<T1>::trilinearTime(curr_pt,
                                                                nodes[itmp[0]],
                                                                nodes[itmp[1]],
                                                                nodes[itmp[2]],
                                                                nodes[itmp[3]],
                                                                threadNo);
                    g = grad3d->compute(curr_pt, curr_t, nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(4);
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    ref_pt[0] = &(nodes[itmp[0]]);
                    ref_pt[1] = &(nodes[itmp[1]]);
                    ref_pt[2] = &(nodes[itmp[2]]);
                    ref_pt[3] = &(nodes[itmp[3]]);
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }
                checkCloseToTx(curr_pt, g, cellNo, Tx, txCell);

                std::array<T2,3> ind[4] = {
                    {itmp[0], itmp[1], itmp[2]},
                    {itmp[0], itmp[1], itmp[3]},
                    {itmp[0], itmp[2], itmp[3]},
                    {itmp[1], itmp[2], itmp[3]}

                };
                for ( size_t n=0; n<4; ++n ) {
                    std::sort( ind[n].begin(), ind[n].end() );
                }

                bool foundIntersection = false;
                for ( size_t n=0; n<4; ++n ) {

                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                             ind[n][1], ind[n][2],
                                                             pt_i);

                    if ( !foundIntersection )
                        continue;

                    curr_pt = pt_i;

                    bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);

                    r_data.push_back( curr_pt );

                    if ( break_flag ) break;

                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath within cell failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_data.resize(1);
                    r_data[0] = Rx;
                    reachedTx = true;
                }
            }

            if ( onNode ) {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        if ( curr_pt.getDistance( Tx[nt] ) < minDist ) {
                            reachedTx = true;
                            break;
                        }
                    } else if ( txOnEdge[nt] ) {
                        if ( curr_pt.getDistance(nodes[txEdges[nt][0]]) < minDist ||
                            curr_pt.getDistance(nodes[txEdges[nt][1]]) < minDist ) {

                            r_data.push_back(Tx[nt]);
                            reachedTx = true;
                            break;
                        }
                    } else if ( txOnFace[nt] ) {
                        if ( curr_pt.getDistance(nodes[txFaces[nt][0]]) < minDist ||
                            curr_pt.getDistance(nodes[txFaces[nt][1]]) < minDist ||
                            curr_pt.getDistance(nodes[txFaces[nt][2]]) < minDist ) {

                            r_data.push_back(Tx[nt]);
                            reachedTx = true;
                            break;
                        }
                    }
                }
            } if ( onEdge ) {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        if ( txNode[nt] == edgeNodes[0] || txNode[nt] == edgeNodes[1] ) {
                            r_data.push_back( Tx[nt] );
                            reachedTx = true;
                            break;
                        }
                    } else {
                        std::array<T2,4> itmp = getPrimary(txCell[nt]);
                        // find adjacent cells
                        const T2 ind[6][2] = {
                            {itmp[0], itmp[1]},
                            {itmp[0], itmp[2]},
                            {itmp[0], itmp[3]},
                            {itmp[1], itmp[2]},
                            {itmp[1], itmp[3]},
                            {itmp[2], itmp[3]} };
                        for ( size_t ne=0; ne<6; ++ne ) {
                            if ( (ind[ne][0] == edgeNodes[0] && ind[ne][1] == edgeNodes[1]) ||
                                (ind[ne][0] == edgeNodes[1] && ind[ne][1] == edgeNodes[0]) ) {
                                r_data.push_back( Tx[nt] );
                                reachedTx = true;
                                break;
                            }
                        }
                    }
                }
            } else {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        for ( auto nc=nodes[txNode[nt]].getOwners().begin(); nc!=nodes[txNode[nt]].getOwners().end(); ++nc ) {
                            if ( cellNo == *nc ) {
                                r_data.push_back( Tx[nt] );
                                reachedTx = true;
                                break;
                            }
                        }
                    } else {
                        if ( cellNo == txCell[nt] ) {
                            r_data.push_back( Tx[nt] );
                            reachedTx = true;
                        } else {
                            for ( size_t nn=0; nn<txNeighborCells[nt].size(); ++nn ) {
                                if ( cellNo == txNeighborCells[nt][nn] ) {
                                    std::array<T2,4> itmp = getPrimary(txCell[nt]);
                                    std::array<T2,3> ind[4] = {
                                        { { itmp[0], itmp[1], itmp[2] } },
                                        { { itmp[0], itmp[1], itmp[3] } },
                                        { { itmp[0], itmp[2], itmp[3] } },
                                        { { itmp[1], itmp[2], itmp[3] } }
                                    };

                                    bool found = false;
                                    for ( size_t n=0; n<4; ++n ) {
                                        std::sort( ind[n].begin(), ind[n].end() );
                                        if ( faceNodes == ind[n] ) {
                                            found = true;
                                            break;
                                        }
                                    }
                                    if ( found ) {
                                        r_data.push_back( Tx[nt] );
                                        reachedTx = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if ( reachedTx ) break;
                }
            }
        }

        delete grad3d;
    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::getRaypath(const std::vector<sxyz<T1>>& Tx,
                                          const std::vector<T1>& t0,
                                          const sxyz<T1> &Rx,
                                          std::vector<sxyz<T1>> &r_data,
                                          T1 &tt,
                                          const size_t threadNo) const {

        T1 minDist = small;
        r_data.emplace_back( Rx );
        tt = 0.0;

        for ( size_t ns=0; ns<Tx.size(); ++ns ) {
            if ( Rx == Tx[ns] ) {
                tt = t0[ns];
                return;
            }
        }

        std::vector<bool> txOnNode( Tx.size(), false );
        std::vector<bool> txOnEdge( Tx.size(), false );
        std::vector<bool> txOnFace( Tx.size(), false );
        std::vector<T2> txNode( Tx.size() );
        std::vector<T2> txCell( Tx.size() );
        std::vector<std::array<T2,2>> txEdges( Tx.size() );
        std::vector<std::array<T2,3>> txFaces( Tx.size() );
        std::vector<std::vector<T2>> txNeighborCells( Tx.size() );
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            for ( T2 nn=0; nn<nodes.size(); ++nn ) {
                if ( nodes[nn].isPrimary() ) {
                    if ( nodes[nn] == Tx[nt] ) {
                        txOnNode[nt] = true;
                        txNode[nt] = nn;
                        break;
                    }
                }
            }
        }
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            if ( !txOnNode[nt] ) {
                txCell[nt] = getCellNo( Tx[nt] );

                std::array<T2,4> itmp = getPrimary(txCell[nt]);
                // find adjacent cells
                T2 ind[6][2] = {
                    {itmp[0], itmp[1]},
                    {itmp[0], itmp[2]},
                    {itmp[0], itmp[3]},
                    {itmp[1], itmp[2]},
                    {itmp[1], itmp[3]},
                    {itmp[2], itmp[3]} };

                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    for ( auto nc0=nodes[ind[nedge][0]].getOwners().begin(); nc0!=nodes[ind[nedge][0]].getOwners().end(); ++nc0 ) {
                        if ( std::find(nodes[ind[nedge][1]].getOwners().begin(), nodes[ind[nedge][1]].getOwners().end(), *nc0)!=nodes[ind[nedge][1]].getOwners().end() )
                            txNeighborCells[nt].push_back( *nc0 );
                    }
                }
                // check if on edge
                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    if ( distSqPointToSegment( &nodes[ind[nedge][0]], &nodes[ind[nedge][1]], Tx[nt]) < small2 ) {
                        txOnEdge[nt] = true;
                        txEdges[nt][0] = ind[nedge][0];
                        txEdges[nt][1] = ind[nedge][1];
                        break;
                    }
                }
                if ( !txOnEdge[nt] ) {
                    // check if on face
                    const T2 indf[4][3] = {
                        {itmp[0], itmp[1], itmp[2]},
                        {itmp[0], itmp[1], itmp[3]},
                        {itmp[0], itmp[2], itmp[3]},
                        {itmp[1], itmp[2], itmp[3]} };
                    for ( size_t nface=0; nface<4; ++nface ) {
                        if ( testInTriangle(&nodes[indf[nface][0]], &nodes[indf[nface][1]], &nodes[indf[nface][2]], Tx[nt]) ) {
                            txOnFace[nt] = true;
                            txFaces[nt][0] = indf[nface][0];
                            txFaces[nt][1] = indf[nface][1];
                            txFaces[nt][2] = indf[nface][2];
                            break;
                        }
                    }
                }
            }
        }

        T2 cellNo=0, nodeNo=0;
        sxyz<T1> curr_pt( Rx );

        bool onNode = false;
        bool onEdge = false;
        bool onFace = false;
        std::array<T2,2> edgeNodes{ {0, 0} };
        std::array<T2,3> faceNodes{ {0, 0, 0} };
        std::array<T2,3> faceNodesStart{ {0, 0, 0} };

        Grad3D<T1,NODE>* grad3d = nullptr;
        if ( rp_method == 0 ) {
            grad3d = new Grad3D_ls_fo<T1,NODE>();
        } else if ( rp_method == 1 ) {
            grad3d = new Grad3D_ls_so<T1,NODE>();
        } else if ( rp_method == 2 ) {
            grad3d = new Grad3D_ab<T1,NODE>();
        }
        bool reachedTx = false;

        for ( T2 nn=0; nn<nodes.size(); ++nn ) {
            if ( nodes[nn].getDistance( curr_pt ) < small ) {
                nodeNo = nn;
                onNode = true;
                break;
            }
        }
        if ( !onNode ) {
            cellNo = getCellNo( curr_pt );

            std::array<T2,4> itmp = getPrimary(cellNo);
            T2 ind[6][2] = {
                {itmp[0], itmp[1]},
                {itmp[0], itmp[2]},
                {itmp[0], itmp[3]},
                {itmp[1], itmp[2]},
                {itmp[1], itmp[3]},
                {itmp[2], itmp[3]} };

            for ( size_t n=0; n<6; ++n ) {
                if ( areCollinear(curr_pt, nodes[ind[n][0]], nodes[ind[n][1]]) ) {
                    onEdge = true;
                    edgeNodes[0] = ind[n][0];
                    edgeNodes[1] = ind[n][1];
                    break;
                }
            }
        }
        if ( !onNode && !onEdge ) {
            std::array<T2,4> itmp = getPrimary(cellNo);
            std::array<T2,3> ind[4] = {
                {itmp[0], itmp[1], itmp[2]},
                {itmp[0], itmp[1], itmp[3]},
                {itmp[0], itmp[2], itmp[3]},
                {itmp[1], itmp[2], itmp[3]}};
            for ( size_t n=0; n<4; ++n ) {
                std::sort( ind[n].begin(), ind[n].end() );
            }

            for ( size_t n=0; n<4; ++n ) {
                if ( areCoplanar(curr_pt, nodes[ind[n][0]], nodes[ind[n][1]], nodes[ind[n][2]]) ) {
                    onFace = true;
                    faceNodesStart = ind[n];
                    break;
                }
            }
        }

        for ( size_t nt=0; nt<txCell.size(); ++nt ) {
            if ( cellNo == txCell[nt] ) {
                tt += t0[nt] + slowness[cellNo] * r_data.back().getDistance( Tx[nt] );
                r_data.push_back( Tx[nt] );
                reachedTx = true;
                break;
            }
        }

        sxyz<T1> g;
        while ( reachedTx == false ) {

            if ( onNode ) {

                if ( rp_method < 2 ) {
                    // find cells common to edge
                    std::set<NODE*> nnodes;
                    for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                        getNeighborNodes(*nc, nnodes);
                    }
                    // compute gradient with nodes from all common cells
                    g = grad3d->compute(curr_pt, nodes[nodeNo].getTT(threadNo), nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(1);
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    ref_pt[0] = &(nodes[nodeNo]);
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }

                // find cell for which gradient intersect opposing face
                bool foundIntersection = false;
                for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {

                    std::array<T2,3> nb;
                    size_t n=0;
                    for (auto nn=this->neighbors[*nc].begin(); nn!=this->neighbors[*nc].end(); ++nn ) {
                        if ( *nn != nodeNo && nodes[*nn].isPrimary() ) {
                            nb[n++] = *nn;
                        }
                    }
                    std::sort(nb.begin(), nb.end());

                    foundIntersection = intersectVecTriangle( nodeNo, g, nb[0], nb[1], nb[2], curr_pt);
                    if ( !foundIntersection ) {
                        continue;
                    }

                    //  check if cell is (one of) TxCell(s)
                    for (size_t nt=0; nt<Tx.size(); ++nt) {
                        if ( *nc == txCell[nt] ) {
                            tt += t0[nt] + slowness[*nc] * r_data.back().getDistance( Tx[nt] );
                            r_data.push_back( Tx[nt] );
                            reachedTx = true;
                            break;
                        }
                    }
                    if ( reachedTx ) {
                        break;
                    }

                    bool break_flag = check_pt_location(curr_pt, nb, onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);

                    tt += slowness[*nc] * r_data.back().getDistance( curr_pt );
                    r_data.push_back( curr_pt );

                    if ( break_flag ) break;

                    // find next cell
                    cellNo = findAdjacentCell1(faceNodes, nodeNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }

                if ( foundIntersection == false ) {
                    // projet gradient on face and find intersection on next edge
                    sxyz<T1> pt_i;
                    foundIntersection = projectOnFace(curr_pt, nodeNo, g, edgeNodes, pt_i);

                    if ( foundIntersection == false ) {
                        std::cout << "\n\nWarning: finding raypath on node failed to converge for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        tt = 0.0;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                        break;
                    }
                    // find which cell we are in
                    for ( size_t nc=0; nc<tetrahedra.size(); ++nc ) {
                        std::array<T2,4> itet = {tetrahedra[nc].i[0],
                            tetrahedra[nc].i[1],
                            tetrahedra[nc].i[2],
                            tetrahedra[nc].i[3]};
                        // because we are at the surface, there is only one cell with nodeNo & edgeNodes
                        if (std::find(itet.begin(), itet.end(), nodeNo) != itet.end() &&
                            std::find(itet.begin(), itet.end(), edgeNodes[0]) != itet.end() &&
                            std::find(itet.begin(), itet.end(), edgeNodes[1]) != itet.end()) {
                            cellNo = static_cast<T2>(nc);
                            break;
                        }
                    }

                    //  check if cell is (one of) TxCell(s)
                    for (size_t nt=0; nt<Tx.size(); ++nt) {
                        if ( cellNo == txCell[nt] ) {

                            tt += t0[nt] + slowness[cellNo] * r_data.back().getDistance( Tx[nt] );
                            r_data.push_back( Tx[nt] );
                            reachedTx = true;
                            break;
                        }
                    }
                    if ( reachedTx ) {
                        break;
                    }
                    T2 prevNode = nodeNo;

                    // we might be on one of the nodes
                    if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[0];
                        pt_i = nodes[edgeNodes[0]];
                        onEdge = false;
                    } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[1];
                        pt_i = nodes[edgeNodes[1]];
                        onEdge = false;
                    } else {
                        onEdge = true;
                        onNode = false;
                    }

                    T1 dt;
                    // find traversed cell
                    if ( onNode ) {
                        // propagation was along edge
                        dt = nodes[nodeNo].getTT(threadNo) - nodes[prevNode].getTT(threadNo);
                    } else { // onEdge
                        T2 traversedCell = 0;
                        for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                            if ( std::find( nodes[edgeNodes[0]].getOwners().begin(), nodes[edgeNodes[0]].getOwners().end(), *nc ) != nodes[edgeNodes[0]].getOwners().end() &&
                                std::find( nodes[edgeNodes[1]].getOwners().begin(), nodes[edgeNodes[1]].getOwners().end(), *nc ) != nodes[edgeNodes[1]].getOwners().end() ) {
                                traversedCell = *nc;
                                break;
                            }
                        }
                        dt = slowness[traversedCell] * r_data.back().getDistance( curr_pt );
                    }

                    curr_pt = pt_i;

                    tt += dt;
                    r_data.push_back( curr_pt );
                }
            } else if ( onEdge ) {

                // find cells common to edge
                std::vector<T2> cells;
                for ( auto nc0=nodes[edgeNodes[0]].getOwners().begin(); nc0!=nodes[edgeNodes[0]].getOwners().end(); ++nc0 ) {
                    if ( std::find(nodes[edgeNodes[1]].getOwners().begin(), nodes[edgeNodes[1]].getOwners().end(), *nc0)!=nodes[edgeNodes[1]].getOwners().end() ) {
                        cells.push_back( *nc0 );
                    }
                }
                if ( rp_method < 2 ) {
                    std::set<NODE*> nnodes;
                    for (size_t n=0; n<cells.size(); ++n ) {
                        getNeighborNodes(cells[n], nnodes);
                    }
                    T1 d01 = nodes[edgeNodes[0]].getDistance(nodes[edgeNodes[1]]);
                    T1 w0 = curr_pt.getDistance(nodes[edgeNodes[1]]) / d01;
                    T1 w1 = curr_pt.getDistance(nodes[edgeNodes[0]]) / d01;
                    T1 curr_t = nodes[edgeNodes[0]].getTT(threadNo)*w0 + nodes[edgeNodes[1]].getTT(threadNo)*w1;
                    g = grad3d->compute(curr_pt, curr_t, nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(2);
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    ref_pt[0] = &(nodes[edgeNodes[0]]);
                    ref_pt[1] = &(nodes[edgeNodes[1]]);
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }
                checkCloseToTx(curr_pt, g, edgeNodes, Tx, txCell);

                bool foundIntersection=false;
                for (size_t n=0; n<cells.size(); ++n ) {

                    T2 testCellNo = cells[n];

                    // there are 2 faces that might be intersected
                    std::array<T2,2> edgeNodes2;
                    size_t n2=0;
                    for ( auto nn=this->neighbors[testCellNo].begin(); nn!= this->neighbors[testCellNo].end(); ++nn ) {
                        if ( *nn!=edgeNodes[0] && *nn!=edgeNodes[1] && nodes[*nn].isPrimary() ) {
                            edgeNodes2[n2++] = *nn;
                        }
                    }

                    sxyz<T1> pt_i;
                    T2 itmpNode;
                    foundIntersection = intersectVecTriangle(curr_pt, g,
                                                             edgeNodes[0],
                                                             edgeNodes2[0],
                                                             edgeNodes2[1], pt_i);
                    itmpNode = edgeNodes[0];
                    if ( !foundIntersection ) {
                        foundIntersection = intersectVecTriangle(curr_pt, g,
                                                                 edgeNodes[1],
                                                                 edgeNodes2[0],
                                                                 edgeNodes2[1], pt_i);
                        itmpNode = edgeNodes[1];
                    }
                    if ( !foundIntersection ) {
                        continue;
                    }

                    cellNo = testCellNo;
                    curr_pt = pt_i;

                    bool break_flag = check_pt_location(curr_pt, this->neighbors[cellNo],
                                                        {itmpNode, edgeNodes2[0], edgeNodes2[1]},
                                                        onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);

                    tt += slowness[cellNo] * r_data.back().getDistance( curr_pt );
                    r_data.push_back( curr_pt );

                    if ( break_flag ) break;

                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                if ( foundIntersection == false ) {
                    // we must be going outside the mesh
                    // hack: project gradient on face and find intersection
#ifdef DEBUG_RP
                    std::cout << "\n\nWarning: raypath (onEdge) likely going outside mesh for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << '\n';
                    std::cout << "         Projecting gradient on external face and resuming" << std::endl;
#endif
                    sxyz<T1> pt_i;
                    std::array<T2,2> pen = edgeNodes;
                    g = projectOnFace(curr_pt, g, edgeNodes, cells, pt_i);
                    if ( g.x==0.0 && g.y==0.0 && g.z==0.0 ) {
                        foundIntersection = false;
                    } else {
                        foundIntersection = true;
                    }

                    if ( foundIntersection == false || curr_pt == pt_i ) {
                        std::cout << "\n\nWarning: finding raypath on edge failed to converge for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                        break;
                    }
                    // we might be on one of the nodes
                    if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[0];
                        pt_i = nodes[edgeNodes[0]];
                        onEdge = false;
                    } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[1];
                        pt_i = nodes[edgeNodes[1]];
                        onEdge = false;
                    }

                    T1 dt;
                    // find traversed cell
                    if ( onNode ) {
                        T2 traversedCell = 0;
                        for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                            if ( std::find( nodes[pen[0]].getOwners().begin(), nodes[pen[0]].getOwners().end(), *nc ) != nodes[pen[0]].getOwners().end() &&
                                std::find( nodes[pen[1]].getOwners().begin(), nodes[pen[1]].getOwners().end(), *nc ) != nodes[pen[1]].getOwners().end() ) {
                                traversedCell = *nc;
                                break;
                            }
                        }
                        dt = slowness[traversedCell] * r_data.back().getDistance( curr_pt );
                    } else {
                        T2 traversedCell = 0;
                        std::array<T2,3> fn;
                        fn[0] = pen[0];
                        fn[1] = pen[1];
                        if ( edgeNodes[0] != fn[0] && edgeNodes[0] != fn[1]) {
                            fn[2] = edgeNodes[0];
                        } else {
                            fn[2] = edgeNodes[1];
                        }
                        for ( auto nc=nodes[fn[0]].getOwners().begin(); nc!=nodes[fn[0]].getOwners().end(); ++nc ) {
                            if ( std::find( nodes[fn[1]].getOwners().begin(), nodes[fn[1]].getOwners().end(), *nc ) != nodes[fn[1]].getOwners().end() &&
                                std::find( nodes[fn[2]].getOwners().begin(), nodes[fn[2]].getOwners().end(), *nc ) != nodes[fn[2]].getOwners().end() ) {
                                traversedCell = *nc;
                                break;
                            }
                        }
                        dt = slowness[traversedCell] * r_data.back().getDistance( curr_pt );
                    }

                    curr_pt = pt_i;
                    tt += dt;
                    r_data.push_back( curr_pt );
                }
            } else if ( onFace ) {

                std::array<T2,4> itmp = getPrimary(cellNo);
                if ( rp_method < 2 ) {
                    std::set<NODE*> nnodes;
                    getNeighborNodes(cellNo, nnodes);
                    T1 curr_t;
                    if ( r_data.size() <= 1 ) {
                        curr_t = Interpolator<T1>::trilinearTime(curr_pt,
                                                                 nodes[itmp[0]],
                                                                 nodes[itmp[1]],
                                                                 nodes[itmp[2]],
                                                                 nodes[itmp[3]],
                                                                 threadNo);
                    } else {
                        curr_t = Interpolator<T1>::bilinearTime(curr_pt,
                                                                nodes[faceNodes[0]],
                                                                nodes[faceNodes[1]],
                                                                nodes[faceNodes[2]],
                                                                threadNo);
                    }
                    g = grad3d->compute(curr_pt, curr_t, nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(3);
                    if ( r_data.size() <= 1 ) {
                        ref_pt[0] = &(nodes[itmp[0]]);
                        ref_pt[1] = &(nodes[itmp[1]]);
                        ref_pt[2] = &(nodes[itmp[2]]);
                        ref_pt.push_back( &(nodes[itmp[3]]) );
                    } else {
                        ref_pt[0] = &(nodes[faceNodes[0]]);
                        ref_pt[1] = &(nodes[faceNodes[1]]);
                        ref_pt[2] = &(nodes[faceNodes[2]]);
                    }
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }
                checkCloseToTx(curr_pt, g, cellNo, Tx, txCell);

                std::array<T2,3> ind[4] = {
                    {itmp[0], itmp[1], itmp[2]},
                    {itmp[0], itmp[1], itmp[3]},
                    {itmp[0], itmp[2], itmp[3]},
                    {itmp[1], itmp[2], itmp[3]}};
                for ( size_t n=0; n<4; ++n ) {
                    std::sort( ind[n].begin(), ind[n].end() );
                }
                // there are 3 faces that might be intersected

                bool foundIntersection = false;
                for ( size_t n=0; n<4; ++n ) {
                    if ( ind[n] == faceNodes ) continue;

                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                             ind[n][1], ind[n][2],
                                                             pt_i);

                    if ( !foundIntersection )
                        continue;

                    curr_pt = pt_i;

                    bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);

                    tt += slowness[cellNo] * r_data.back().getDistance( curr_pt );
                    r_data.push_back( curr_pt );

                    if ( break_flag ) break;

                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }

                if ( foundIntersection == false ) {

                    if ( (faceNodes[0] == faceNodes[1]) && (faceNodes[0] == faceNodes[2]) ) {
                        // we have started on a face and cellNo was incorrectly picked
                        // find adjacent cell
                        cellNo = findAdjacentCell2(faceNodesStart, cellNo);
                    } else {
                        // we must be on an face with gradient pointing slightly outward tetrahedron
                        // return in other cell but keep gradient
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                    }

                    std::array<T2,4> itmp = getPrimary(cellNo);
                    ind[0] = {itmp[0], itmp[1], itmp[2]};
                    ind[1] = {itmp[0], itmp[1], itmp[3]};
                    ind[2] = {itmp[0], itmp[2], itmp[3]};
                    ind[3] = {itmp[1], itmp[2], itmp[3]};

                    for ( size_t n=0; n<4; ++n ) {
                        std::sort( ind[n].begin(), ind[n].end() );
                    }

                    for ( size_t n=0; n<4; ++n ) {
                        if ( ind[n] == faceNodes ) continue;

                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                 ind[n][1], ind[n][2],
                                                                 pt_i);

                        if ( !foundIntersection ) {
                            continue;
                        }
                        curr_pt = pt_i;

                        bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                            nodeNo, onEdge, edgeNodes,
                                                            onFace, faceNodes);

                        tt += slowness[cellNo] * r_data.back().getDistance( curr_pt );
                        r_data.push_back( curr_pt );

                        if ( break_flag ) break;

                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                            << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                            r_data.resize(1);
                            r_data[0] = Rx;
                            reachedTx = true;
                        }
                        break;
                    }
                }
                if ( foundIntersection == false ) {
                    // we must be going outside the mesh
                    // hack: project gradient on face and continue
#ifdef DEBUG_RP
                    std::cout << "\n\nWarning: raypath (onFace) likely going outside mesh for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << '\n';
                    std::cout << "         Projecting gradient on external face and resuming" << std::endl;
#endif
                    g = projectOnFace(g, faceNodes);

                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecEdge(curr_pt, g, faceNodes, pt_i, edgeNodes);

                    if ( foundIntersection == false ) {
                        std::cout << "\n\nWarning: finding raypath on face failed to converge for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                        break;

                    }
                    // we might be on one of the nodes
                    if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[0];
                        pt_i = nodes[edgeNodes[0]];
                        onEdge = false;
                        onFace = false;
                    } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[1];
                        pt_i = nodes[edgeNodes[1]];
                        onEdge = false;
                        onFace = false;
                    } else {
                        onEdge = true;
                        onFace = false;
                    }
                    curr_pt = pt_i;
                    T2 traversedCell = 0;
                    for ( auto nc=nodes[faceNodes[0]].getOwners().begin(); nc!=nodes[faceNodes[0]].getOwners().end(); ++nc ) {
                        if ( std::find( nodes[faceNodes[1]].getOwners().begin(), nodes[faceNodes[1]].getOwners().end(), *nc ) != nodes[faceNodes[1]].getOwners().end() &&
                            std::find( nodes[faceNodes[2]].getOwners().begin(), nodes[faceNodes[2]].getOwners().end(), *nc ) != nodes[faceNodes[2]].getOwners().end() ) {
                            traversedCell = *nc;
                            break;
                        }
                    }

                    tt += slowness[traversedCell] * r_data.back().getDistance( curr_pt );
                    r_data.push_back( curr_pt );
                }
            } else { // at Rx, somewhere in a tetrahedron

                std::array<T2,4> itmp = getPrimary(cellNo);
                if ( rp_method < 2 ) {
                    std::set<NODE*> nnodes;
                    getNeighborNodes(cellNo, nnodes);
                    T1 curr_t = Interpolator<T1>::trilinearTime(curr_pt,
                                                                nodes[itmp[0]],
                                                                nodes[itmp[1]],
                                                                nodes[itmp[2]],
                                                                nodes[itmp[3]],
                                                                threadNo);
                    g = grad3d->compute(curr_pt, curr_t, nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(4);
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    ref_pt[0] = &(nodes[itmp[0]]);
                    ref_pt[1] = &(nodes[itmp[1]]);
                    ref_pt[2] = &(nodes[itmp[2]]);
                    ref_pt[3] = &(nodes[itmp[3]]);
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }
                checkCloseToTx(curr_pt, g, cellNo, Tx, txCell);

                std::array<T2,3> ind[4] = {
                    {itmp[0], itmp[1], itmp[2]},
                    {itmp[0], itmp[1], itmp[3]},
                    {itmp[0], itmp[2], itmp[3]},
                    {itmp[1], itmp[2], itmp[3]}};
                for ( size_t n=0; n<4; ++n ) {
                    std::sort( ind[n].begin(), ind[n].end() );
                }

                bool foundIntersection = false;
                for ( size_t n=0; n<4; ++n ) {

                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                             ind[n][1], ind[n][2],
                                                             pt_i);

                    if ( !foundIntersection )
                        continue;

                    curr_pt = pt_i;

                    bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);

                    tt += slowness[cellNo] * r_data.back().getDistance( curr_pt );
                    r_data.push_back( curr_pt );

                    if ( break_flag ) break;

                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        r_data.resize(1);
                        r_data[0] = Rx;
                        reachedTx = true;
                    }
                    break;
                }
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath within cell failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    r_data.resize(1);
                    r_data[0] = Rx;
                    reachedTx = true;
                }
            }

            if ( onNode ) {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        if ( curr_pt.getDistance( Tx[nt] ) < minDist ) {
                            tt += t0[nt];
                            reachedTx = true;
                            break;
                        }
                    } else if ( txOnEdge[nt] ) {
                        if ( curr_pt.getDistance(nodes[txEdges[nt][0]]) < minDist ||
                            curr_pt.getDistance(nodes[txEdges[nt][1]]) < minDist ) {

                            tt += t0[nt] + slowness[txCell[nt]] * r_data.back().getDistance( Tx[nt] );
                            reachedTx = true;
                            r_data.push_back(Tx[nt]);
                            break;
                        }
                    } else if ( txOnFace[nt] ) {
                        if ( curr_pt.getDistance(nodes[txFaces[nt][0]]) < minDist ||
                            curr_pt.getDistance(nodes[txFaces[nt][1]]) < minDist ||
                            curr_pt.getDistance(nodes[txFaces[nt][2]]) < minDist ) {

                            tt += t0[nt] + slowness[txCell[nt]] * r_data.back().getDistance( Tx[nt] );
                            reachedTx = true;
                            r_data.push_back(Tx[nt]);
                            break;
                        }
                    }
                }
            } if ( onEdge ) {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        if ( txNode[nt] == edgeNodes[0] || txNode[nt] == edgeNodes[1] ) {
                            tt += t0[nt] + slowness[cellNo] * r_data.back().getDistance( Tx[nt] );
                            r_data.push_back(Tx[nt]);
                            reachedTx = true;
                            break;
                        }
                    } else {
                        std::array<T2,4> itmp = getPrimary(txCell[nt]);
                        // find adjacent cells
                        const T2 ind[6][2] = {
                            {itmp[0], itmp[1]},
                            {itmp[0], itmp[2]},
                            {itmp[0], itmp[3]},
                            {itmp[1], itmp[2]},
                            {itmp[1], itmp[3]},
                            {itmp[2], itmp[3]} };
                        for ( size_t ne=0; ne<6; ++ne ) {
                            if ( (ind[ne][0] == edgeNodes[0] && ind[ne][1] == edgeNodes[1]) ||
                                (ind[ne][0] == edgeNodes[1] && ind[ne][1] == edgeNodes[0]) ) {

                                tt += t0[nt] + slowness[txCell[nt]] * r_data.back().getDistance( Tx[nt] );
                                r_data.push_back(Tx[nt]);
                                reachedTx = true;
                                break;
                            }
                        }
                    }
                }
            } else {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        for ( auto nc=nodes[txNode[nt]].getOwners().begin(); nc!=nodes[txNode[nt]].getOwners().end(); ++nc ) {
                            if ( cellNo == *nc ) {
                                tt += t0[nt] + slowness[cellNo] * r_data.back().getDistance( Tx[nt] );
                                r_data.push_back( Tx[nt] );
                                reachedTx = true;
                                break;
                            }
                        }
                    } else {
                        if ( cellNo == txCell[nt] ) {
                            tt += t0[nt] + slowness[cellNo] * r_data.back().getDistance( Tx[nt] );
                            r_data.push_back( Tx[nt] );
                            reachedTx = true;
                        } else {
                            for ( size_t nn=0; nn<txNeighborCells[nt].size(); ++nn ) {
                                if ( cellNo == txNeighborCells[nt][nn] ) {
                                    std::array<T2,4> itmp = getPrimary(txCell[nt]);
                                    std::array<T2,3> ind[4] = {
                                        { { itmp[0], itmp[1], itmp[2] } },
                                        { { itmp[0], itmp[1], itmp[3] } },
                                        { { itmp[0], itmp[2], itmp[3] } },
                                        { { itmp[1], itmp[2], itmp[3] } }
                                    };

                                    bool found = false;
                                    for ( size_t n=0; n<4; ++n ) {
                                        std::sort( ind[n].begin(), ind[n].end() );
                                        if ( faceNodes == ind[n] ) {
                                            found = true;
                                            break;
                                        }
                                    }
                                    if ( found ) {
                                        tt += t0[nt] + slowness[cellNo] * r_data.back().getDistance( Tx[nt] );
                                        r_data.push_back( Tx[nt] );
                                        reachedTx = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if ( reachedTx ) break;
                }
            }
        }

        delete grad3d;
    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::getRaypath(const std::vector<sxyz<T1>>& Tx,
                                          const std::vector<T1>& t0,
                                          const sxyz<T1> &Rx,
                                          std::vector<siv<T1>> &l_data,
                                          T1 &tt,
                                          const size_t threadNo) const {
        
        T1 minDist = small;
        tt = 0.0;
        l_data.resize(0);
        
        for ( size_t ns=0; ns<Tx.size(); ++ns ) {
            if ( Rx == Tx[ns] ) {
                tt = t0[ns];
                return;
            }
        }
        
        std::vector<bool> txOnNode( Tx.size(), false );
        std::vector<bool> txOnEdge( Tx.size(), false );
        std::vector<bool> txOnFace( Tx.size(), false );
        std::vector<T2> txNode( Tx.size() );
        std::vector<T2> txCell( Tx.size() );
        std::vector<std::array<T2,2>> txEdges( Tx.size() );
        std::vector<std::array<T2,3>> txFaces( Tx.size() );
        std::vector<std::vector<T2>> txNeighborCells( Tx.size() );
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            for ( T2 nn=0; nn<nodes.size(); ++nn ) {
                if ( nodes[nn].isPrimary() ) {
                    if ( nodes[nn] == Tx[nt] ) {
                        txOnNode[nt] = true;
                        txNode[nt] = nn;
                        break;
                    }
                }
            }
        }
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            if ( !txOnNode[nt] ) {
                txCell[nt] = getCellNo( Tx[nt] );
                
                std::array<T2,4> itmp = getPrimary(txCell[nt]);
                // find adjacent cells
                T2 ind[6][2] = {
                    {itmp[0], itmp[1]},
                    {itmp[0], itmp[2]},
                    {itmp[0], itmp[3]},
                    {itmp[1], itmp[2]},
                    {itmp[1], itmp[3]},
                    {itmp[2], itmp[3]} };
                
                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    for ( auto nc0=nodes[ind[nedge][0]].getOwners().begin(); nc0!=nodes[ind[nedge][0]].getOwners().end(); ++nc0 ) {
                        if ( std::find(nodes[ind[nedge][1]].getOwners().begin(), nodes[ind[nedge][1]].getOwners().end(), *nc0)!=nodes[ind[nedge][1]].getOwners().end() )
                            txNeighborCells[nt].push_back( *nc0 );
                    }
                }
                // check if on edge
                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    if ( distSqPointToSegment( &nodes[ind[nedge][0]], &nodes[ind[nedge][1]], Tx[nt]) < small2 ) {
                        txOnEdge[nt] = true;
                        txEdges[nt][0] = ind[nedge][0];
                        txEdges[nt][1] = ind[nedge][1];
                        break;
                    }
                }
                if ( !txOnEdge[nt] ) {
                    // check if on face
                    const T2 indf[4][3] = {
                        {itmp[0], itmp[1], itmp[2]},
                        {itmp[0], itmp[1], itmp[3]},
                        {itmp[0], itmp[2], itmp[3]},
                        {itmp[1], itmp[2], itmp[3]} };
                    for ( size_t nface=0; nface<4; ++nface ) {
                        if ( testInTriangle(&nodes[indf[nface][0]], &nodes[indf[nface][1]], &nodes[indf[nface][2]], Tx[nt]) ) {
                            txOnFace[nt] = true;
                            txFaces[nt][0] = indf[nface][0];
                            txFaces[nt][1] = indf[nface][1];
                            txFaces[nt][2] = indf[nface][2];
                            break;
                        }
                    }
                }
            }
        }
        
        T2 cellNo=0, nodeNo=0;
        sxyz<T1> curr_pt( Rx );
        sxyz<T1> prev_pt( Rx );
        
        bool onNode = false;
        bool onEdge = false;
        bool onFace = false;
        std::array<T2,2> edgeNodes{ {0, 0} };
        std::array<T2,3> faceNodes{ {0, 0, 0} };
        std::array<T2,3> faceNodesStart{ {0, 0, 0} };
        
        Grad3D<T1,NODE>* grad3d = nullptr;
        if ( rp_method == 0 ) {
            grad3d = new Grad3D_ls_fo<T1,NODE>();
        } else if ( rp_method == 1 ) {
            grad3d = new Grad3D_ls_so<T1,NODE>();
        } else if ( rp_method == 2 ) {
            grad3d = new Grad3D_ab<T1,NODE>();
        }
        bool reachedTx = false;
        
        for ( T2 nn=0; nn<nodes.size(); ++nn ) {
            if ( nodes[nn].getDistance( curr_pt ) < small ) {
                nodeNo = nn;
                onNode = true;
                break;
            }
        }
        if ( !onNode ) {
            cellNo = getCellNo( curr_pt );
            
            std::array<T2,4> itmp = getPrimary(cellNo);
            T2 ind[6][2] = {
                {itmp[0], itmp[1]},
                {itmp[0], itmp[2]},
                {itmp[0], itmp[3]},
                {itmp[1], itmp[2]},
                {itmp[1], itmp[3]},
                {itmp[2], itmp[3]} };
            
            for ( size_t n=0; n<6; ++n ) {
                if ( areCollinear(curr_pt, nodes[ind[n][0]], nodes[ind[n][1]]) ) {
                    onEdge = true;
                    edgeNodes[0] = ind[n][0];
                    edgeNodes[1] = ind[n][1];
                    break;
                }
            }
        }
        if ( !onNode && !onEdge ) {
            std::array<T2,4> itmp = getPrimary(cellNo);
            std::array<T2,3> ind[4] = {
                {itmp[0], itmp[1], itmp[2]},
                {itmp[0], itmp[1], itmp[3]},
                {itmp[0], itmp[2], itmp[3]},
                {itmp[1], itmp[2], itmp[3]}};
            for ( size_t n=0; n<4; ++n ) {
                std::sort( ind[n].begin(), ind[n].end() );
            }
            
            for ( size_t n=0; n<4; ++n ) {
                if ( areCoplanar(curr_pt, nodes[ind[n][0]], nodes[ind[n][1]], nodes[ind[n][2]]) ) {
                    onFace = true;
                    faceNodesStart = ind[n];
                    break;
                }
            }
        }
        
        siv<T1> cell;
        for ( size_t nt=0; nt<txCell.size(); ++nt ) {
            if ( cellNo == txCell[nt] ) {
                cell.i = cellNo;
                cell.v = prev_pt.getDistance( Tx[nt] );
                l_data.push_back( cell );
                tt += t0[nt] + slowness[cellNo] * cell.v;
                reachedTx = true;
                break;
            }
        }
        
        sxyz<T1> g;
        while ( reachedTx == false ) {
            
            if ( onNode ) {
                
                if ( rp_method < 2 ) {
                    // find cells common to edge
                    std::set<NODE*> nnodes;
                    for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                        getNeighborNodes(*nc, nnodes);
                    }
                    // compute gradient with nodes from all common cells
                    g = grad3d->compute(curr_pt, nodes[nodeNo].getTT(threadNo), nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(1);
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    ref_pt[0] = &(nodes[nodeNo]);
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }
                
                // find cell for which gradient intersect opposing face
                bool foundIntersection = false;
                for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                    
                    std::array<T2,3> nb;
                    size_t n=0;
                    for (auto nn=this->neighbors[*nc].begin(); nn!=this->neighbors[*nc].end(); ++nn ) {
                        if ( *nn != nodeNo && nodes[*nn].isPrimary() ) {
                            nb[n++] = *nn;
                        }
                    }
                    std::sort(nb.begin(), nb.end());
                    
                    foundIntersection = intersectVecTriangle( nodeNo, g, nb[0], nb[1], nb[2], curr_pt);
                    if ( !foundIntersection ) {
                        continue;
                    }
                    
                    //  check if cell is (one of) TxCell(s)
                    for (size_t nt=0; nt<Tx.size(); ++nt) {
                        if ( *nc == txCell[nt] ) {
                            cell.i = *nc;
                            cell.v = prev_pt.getDistance( Tx[nt] );
                            l_data.push_back(cell);
                            tt += t0[nt] + slowness[*nc] * cell.v;
                            reachedTx = true;
                            break;
                        }
                    }
                    if ( reachedTx ) {
                        break;
                    }
                    
                    bool break_flag = check_pt_location(curr_pt, nb, onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);
                    
                    cell.i = *nc;
                    cell.v = prev_pt.getDistance( curr_pt );
                    l_data.push_back(cell);
                    tt += slowness[*nc] * cell.v;
                    prev_pt = curr_pt ;
                    
                    if ( break_flag ) break;
                    
                    // find next cell
                    cellNo = findAdjacentCell1(faceNodes, nodeNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        l_data.resize(0);
                        reachedTx = true;
                    }
                    break;
                }
                
                if ( foundIntersection == false ) {
                    // projet gradient on face and find intersection on next edge
                    sxyz<T1> pt_i;
                    foundIntersection = projectOnFace(curr_pt, nodeNo, g, edgeNodes, pt_i);
                    
                    if ( foundIntersection == false ) {
                        std::cout << "\n\nWarning: finding raypath on node failed to converge for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        tt = 0.0;
                        l_data.resize(0);
                        reachedTx = true;
                        break;
                    }
                    // find which cell we are in
                    for ( size_t nc=0; nc<tetrahedra.size(); ++nc ) {
                        std::array<T2,4> itet = {tetrahedra[nc].i[0],
                            tetrahedra[nc].i[1],
                            tetrahedra[nc].i[2],
                            tetrahedra[nc].i[3]};
                        // because we are at the surface, there is only one cell with nodeNo & edgeNodes
                        if (std::find(itet.begin(), itet.end(), nodeNo) != itet.end() &&
                            std::find(itet.begin(), itet.end(), edgeNodes[0]) != itet.end() &&
                            std::find(itet.begin(), itet.end(), edgeNodes[1]) != itet.end()) {
                            cellNo = static_cast<T2>(nc);
                            break;
                        }
                    }
                    
                    //  check if cell is (one of) TxCell(s)
                    for (size_t nt=0; nt<Tx.size(); ++nt) {
                        if ( cellNo == txCell[nt] ) {
                            cell.i = cellNo;
                            cell.v = prev_pt.getDistance( Tx[nt] );
                            l_data.push_back(cell);
                            tt += t0[nt] + slowness[cellNo] * cell.v;
                            reachedTx = true;
                            break;
                        }
                    }
                    if ( reachedTx ) {
                        break;
                    }
                    T2 prevNode = nodeNo;
                    
                    // we might be on one of the nodes
                    if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[0];
                        pt_i = nodes[edgeNodes[0]];
                        onEdge = false;
                    } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[1];
                        pt_i = nodes[edgeNodes[1]];
                        onEdge = false;
                    } else {
                        onEdge = true;
                        onNode = false;
                    }
                    
                    T1 dt;
                    // find traversed cell
                    if ( onNode ) {
                        // propagation was along edge
                        dt = nodes[nodeNo].getTT(threadNo) - nodes[prevNode].getTT(threadNo);
                        
                        // find common cells
                        std::vector<T2> cells;
                        for ( auto nc0=nodes[nodeNo].getOwners().begin(); nc0!=nodes[nodeNo].getOwners().end(); ++nc0 ) {
                            if ( std::find(nodes[prevNode].getOwners().begin(), nodes[prevNode].getOwners().end(), *nc0)!=nodes[prevNode].getOwners().end() ) {
                                cells.push_back( *nc0 );
                            }
                        }
                        // pick fastest cell
                        cell.i = cells[0];
                        for ( auto nc0=1; nc0<cells.size(); ++nc0 ) {
                            if ( slowness[cell.i] > slowness[nc0] ) {
                                cell.i = nc0;
                            }
                        }
                        cell.v = nodes[nodeNo].getDistance(nodes[prevNode]);
                        l_data.push_back(cell);
                    } else { // onEdge
                        T2 traversedCell = 0;
                        for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                            if ( std::find( nodes[edgeNodes[0]].getOwners().begin(), nodes[edgeNodes[0]].getOwners().end(), *nc ) != nodes[edgeNodes[0]].getOwners().end() &&
                                std::find( nodes[edgeNodes[1]].getOwners().begin(), nodes[edgeNodes[1]].getOwners().end(), *nc ) != nodes[edgeNodes[1]].getOwners().end() ) {
                                traversedCell = *nc;
                                break;
                            }
                        }
                        cell.i = traversedCell;
                        cell.v = prev_pt.getDistance( curr_pt );
                        l_data.push_back(cell);
                        dt = slowness[traversedCell] * cell.v;
                    }
                    
                    curr_pt = pt_i;
                    
                    tt += dt;
                    prev_pt = curr_pt;
                }
            } else if ( onEdge ) {
                
                // find cells common to edge
                std::vector<T2> cells;
                for ( auto nc0=nodes[edgeNodes[0]].getOwners().begin(); nc0!=nodes[edgeNodes[0]].getOwners().end(); ++nc0 ) {
                    if ( std::find(nodes[edgeNodes[1]].getOwners().begin(), nodes[edgeNodes[1]].getOwners().end(), *nc0)!=nodes[edgeNodes[1]].getOwners().end() ) {
                        cells.push_back( *nc0 );
                    }
                }
                if ( rp_method < 2 ) {
                    std::set<NODE*> nnodes;
                    for (size_t n=0; n<cells.size(); ++n ) {
                        getNeighborNodes(cells[n], nnodes);
                    }
                    T1 d01 = nodes[edgeNodes[0]].getDistance(nodes[edgeNodes[1]]);
                    T1 w0 = curr_pt.getDistance(nodes[edgeNodes[1]]) / d01;
                    T1 w1 = curr_pt.getDistance(nodes[edgeNodes[0]]) / d01;
                    T1 curr_t = nodes[edgeNodes[0]].getTT(threadNo)*w0 + nodes[edgeNodes[1]].getTT(threadNo)*w1;
                    g = grad3d->compute(curr_pt, curr_t, nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(2);
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    ref_pt[0] = &(nodes[edgeNodes[0]]);
                    ref_pt[1] = &(nodes[edgeNodes[1]]);
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }
                checkCloseToTx(curr_pt, g, edgeNodes, Tx, txCell);
                
                bool foundIntersection=false;
                for (size_t n=0; n<cells.size(); ++n ) {
                    
                    T2 testCellNo = cells[n];
                    
                    // there are 2 faces that might be intersected
                    std::array<T2,2> edgeNodes2;
                    size_t n2=0;
                    for ( auto nn=this->neighbors[testCellNo].begin(); nn!= this->neighbors[testCellNo].end(); ++nn ) {
                        if ( *nn!=edgeNodes[0] && *nn!=edgeNodes[1] && nodes[*nn].isPrimary() ) {
                            edgeNodes2[n2++] = *nn;
                        }
                    }
                    
                    sxyz<T1> pt_i;
                    T2 itmpNode;
                    foundIntersection = intersectVecTriangle(curr_pt, g,
                                                             edgeNodes[0],
                                                             edgeNodes2[0],
                                                             edgeNodes2[1], pt_i);
                    itmpNode = edgeNodes[0];
                    if ( !foundIntersection ) {
                        foundIntersection = intersectVecTriangle(curr_pt, g,
                                                                 edgeNodes[1],
                                                                 edgeNodes2[0],
                                                                 edgeNodes2[1], pt_i);
                        itmpNode = edgeNodes[1];
                    }
                    if ( !foundIntersection ) {
                        continue;
                    }
                    
                    cellNo = testCellNo;
                    curr_pt = pt_i;
                    
                    bool break_flag = check_pt_location(curr_pt, this->neighbors[cellNo],
                                                        {itmpNode, edgeNodes2[0], edgeNodes2[1]},
                                                        onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);
                    
                    cell.i = cellNo;
                    cell.v = prev_pt.getDistance( curr_pt );
                    l_data.push_back(cell);
                    tt += slowness[cellNo] * cell.v;
                    prev_pt = curr_pt;
                    
                    if ( break_flag ) break;
                    
                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        l_data.resize(0);
                        reachedTx = true;
                    }
                    break;
                }
                if ( foundIntersection == false ) {
                    // we must be going outside the mesh
                    // hack: project gradient on face and find intersection
#ifdef DEBUG_RP
                    std::cout << "\n\nWarning: raypath (onEdge) likely going outside mesh for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << '\n';
                    std::cout << "         Projecting gradient on external face and resuming" << std::endl;
#endif
                    sxyz<T1> pt_i;
                    std::array<T2,2> pen = edgeNodes;
                    g = projectOnFace(curr_pt, g, edgeNodes, cells, pt_i);
                    if ( g.x==0.0 && g.y==0.0 && g.z==0.0 ) {
                        foundIntersection = false;
                    } else {
                        foundIntersection = true;
                    }
                    
                    if ( foundIntersection == false || curr_pt == pt_i ) {
                        std::cout << "\n\nWarning: finding raypath on edge failed to converge for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        l_data.resize(0);
                        reachedTx = true;
                        break;
                    }
                    // we might be on one of the nodes
                    if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[0];
                        pt_i = nodes[edgeNodes[0]];
                        onEdge = false;
                    } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[1];
                        pt_i = nodes[edgeNodes[1]];
                        onEdge = false;
                    }
                    
                    T1 dt;
                    // find traversed cell
                    if ( onNode ) {
                        T2 traversedCell = 0;
                        for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
                            if ( std::find( nodes[pen[0]].getOwners().begin(), nodes[pen[0]].getOwners().end(), *nc ) != nodes[pen[0]].getOwners().end() &&
                                std::find( nodes[pen[1]].getOwners().begin(), nodes[pen[1]].getOwners().end(), *nc ) != nodes[pen[1]].getOwners().end() ) {
                                traversedCell = *nc;
                                break;
                            }
                        }
                        cell.i = traversedCell;
                        cell.v = prev_pt.getDistance( curr_pt );
                        l_data.push_back(cell);
                        dt = slowness[traversedCell] * cell.v;
                    } else {
                        T2 traversedCell = 0;
                        std::array<T2,3> fn;
                        fn[0] = pen[0];
                        fn[1] = pen[1];
                        if ( edgeNodes[0] != fn[0] && edgeNodes[0] != fn[1]) {
                            fn[2] = edgeNodes[0];
                        } else {
                            fn[2] = edgeNodes[1];
                        }
                        for ( auto nc=nodes[fn[0]].getOwners().begin(); nc!=nodes[fn[0]].getOwners().end(); ++nc ) {
                            if ( std::find( nodes[fn[1]].getOwners().begin(), nodes[fn[1]].getOwners().end(), *nc ) != nodes[fn[1]].getOwners().end() &&
                                std::find( nodes[fn[2]].getOwners().begin(), nodes[fn[2]].getOwners().end(), *nc ) != nodes[fn[2]].getOwners().end() ) {
                                traversedCell = *nc;
                                break;
                            }
                        }
                        cell.i = traversedCell;
                        cell.v = prev_pt.getDistance( curr_pt );
                        l_data.push_back(cell);
                        dt = slowness[traversedCell] * cell.v;
                    }
                    
                    curr_pt = pt_i;
                    tt += dt;
                    prev_pt = curr_pt;
                }
            } else if ( onFace ) {
                
                std::array<T2,4> itmp = getPrimary(cellNo);
                if ( rp_method < 2 ) {
                    std::set<NODE*> nnodes;
                    getNeighborNodes(cellNo, nnodes);
                    T1 curr_t;
                    if ( l_data.size() == 0 ) {
                        curr_t = Interpolator<T1>::trilinearTime(curr_pt,
                                                                 nodes[itmp[0]],
                                                                 nodes[itmp[1]],
                                                                 nodes[itmp[2]],
                                                                 nodes[itmp[3]],
                                                                 threadNo);
                    } else {
                        curr_t = Interpolator<T1>::bilinearTime(curr_pt,
                                                                nodes[faceNodes[0]],
                                                                nodes[faceNodes[1]],
                                                                nodes[faceNodes[2]],
                                                                threadNo);
                    }
                    g = grad3d->compute(curr_pt, curr_t, nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(3);
                    if ( l_data.size() == 0 ) {
                        ref_pt[0] = &(nodes[itmp[0]]);
                        ref_pt[1] = &(nodes[itmp[1]]);
                        ref_pt[2] = &(nodes[itmp[2]]);
                        ref_pt.push_back( &(nodes[itmp[3]]) );
                    } else {
                        ref_pt[0] = &(nodes[faceNodes[0]]);
                        ref_pt[1] = &(nodes[faceNodes[1]]);
                        ref_pt[2] = &(nodes[faceNodes[2]]);
                    }
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }
                checkCloseToTx(curr_pt, g, cellNo, Tx, txCell);
                
                std::array<T2,3> ind[4] = {
                    {itmp[0], itmp[1], itmp[2]},
                    {itmp[0], itmp[1], itmp[3]},
                    {itmp[0], itmp[2], itmp[3]},
                    {itmp[1], itmp[2], itmp[3]}};
                for ( size_t n=0; n<4; ++n ) {
                    std::sort( ind[n].begin(), ind[n].end() );
                }
                // there are 3 faces that might be intersected
                
                bool foundIntersection = false;
                for ( size_t n=0; n<4; ++n ) {
                    if ( ind[n] == faceNodes ) continue;
                    
                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                             ind[n][1], ind[n][2],
                                                             pt_i);
                    
                    if ( !foundIntersection )
                        continue;
                    
                    curr_pt = pt_i;
                    
                    bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);
                    
                    cell.i = cellNo;
                    cell.v = prev_pt.getDistance( curr_pt );
                    l_data.push_back(cell);
                    tt += slowness[cellNo] * cell.v;
                    prev_pt = curr_pt;
                    
                    if ( break_flag ) break;
                    
                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        l_data.resize(0);
                        reachedTx = true;
                    }
                    break;
                }
                
                if ( foundIntersection == false ) {
                    
                    if ( (faceNodes[0] == faceNodes[1]) && (faceNodes[0] == faceNodes[2]) ) {
                        // we have started on a face and cellNo was incorrectly picked
                        // find adjacent cell
                        cellNo = findAdjacentCell2(faceNodesStart, cellNo);
                    } else {
                        // we must be on an face with gradient pointing slightly outward tetrahedron
                        // return in other cell but keep gradient
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                    }
                    
                    std::array<T2,4> itmp = getPrimary(cellNo);
                    ind[0] = {itmp[0], itmp[1], itmp[2]};
                    ind[1] = {itmp[0], itmp[1], itmp[3]};
                    ind[2] = {itmp[0], itmp[2], itmp[3]};
                    ind[3] = {itmp[1], itmp[2], itmp[3]};
                    
                    for ( size_t n=0; n<4; ++n ) {
                        std::sort( ind[n].begin(), ind[n].end() );
                    }
                    
                    for ( size_t n=0; n<4; ++n ) {
                        if ( ind[n] == faceNodes ) continue;
                        
                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                 ind[n][1], ind[n][2],
                                                                 pt_i);
                        
                        if ( !foundIntersection ) {
                            continue;
                        }
                        curr_pt = pt_i;
                        
                        bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                            nodeNo, onEdge, edgeNodes,
                                                            onFace, faceNodes);
                        
                        cell.i = cellNo;
                        cell.v = prev_pt.getDistance( curr_pt );
                        l_data.push_back(cell);
                        tt += slowness[cellNo] * cell.v;
                        prev_pt = curr_pt;
                        
                        if ( break_flag ) break;
                        
                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                            << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                            l_data.resize(0);
                            reachedTx = true;
                        }
                        break;
                    }
                }
                if ( foundIntersection == false ) {
                    // we must be going outside the mesh
                    // hack: project gradient on face and continue
#ifdef DEBUG_RP
                    std::cout << "\n\nWarning: raypath (onFace) likely going outside mesh for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << '\n';
                    std::cout << "         Projecting gradient on external face and resuming" << std::endl;
#endif
                    g = projectOnFace(g, faceNodes);
                    
                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecEdge(curr_pt, g, faceNodes, pt_i, edgeNodes);
                    
                    if ( foundIntersection == false ) {
                        std::cout << "\n\nWarning: finding raypath on face failed to converge for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        l_data.resize(0);
                        reachedTx = true;
                        break;
                        
                    }
                    // we might be on one of the nodes
                    if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[0];
                        pt_i = nodes[edgeNodes[0]];
                        onEdge = false;
                        onFace = false;
                    } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                        onNode = true;
                        nodeNo = edgeNodes[1];
                        pt_i = nodes[edgeNodes[1]];
                        onEdge = false;
                        onFace = false;
                    } else {
                        onEdge = true;
                        onFace = false;
                    }
                    curr_pt = pt_i;
                    T2 traversedCell = 0;
                    for ( auto nc=nodes[faceNodes[0]].getOwners().begin(); nc!=nodes[faceNodes[0]].getOwners().end(); ++nc ) {
                        if ( std::find( nodes[faceNodes[1]].getOwners().begin(), nodes[faceNodes[1]].getOwners().end(), *nc ) != nodes[faceNodes[1]].getOwners().end() &&
                            std::find( nodes[faceNodes[2]].getOwners().begin(), nodes[faceNodes[2]].getOwners().end(), *nc ) != nodes[faceNodes[2]].getOwners().end() ) {
                            traversedCell = *nc;
                            break;
                        }
                    }
                    
                    cell.i = traversedCell;
                    cell.v = prev_pt.getDistance( curr_pt );
                    l_data.push_back(cell);
                    tt += slowness[traversedCell] * cell.v;
                    prev_pt = curr_pt;
                }
            } else { // at Rx, somewhere in a tetrahedron
                
                std::array<T2,4> itmp = getPrimary(cellNo);
                if ( rp_method < 2 ) {
                    std::set<NODE*> nnodes;
                    getNeighborNodes(cellNo, nnodes);
                    T1 curr_t = Interpolator<T1>::trilinearTime(curr_pt,
                                                                nodes[itmp[0]],
                                                                nodes[itmp[1]],
                                                                nodes[itmp[2]],
                                                                nodes[itmp[3]],
                                                                threadNo);
                    g = grad3d->compute(curr_pt, curr_t, nnodes, threadNo);
                } else {
                    std::vector<NODE*> ref_pt(4);
                    std::vector<std::vector<std::array<NODE*,3>>> opp_pts;
                    ref_pt[0] = &(nodes[itmp[0]]);
                    ref_pt[1] = &(nodes[itmp[1]]);
                    ref_pt[2] = &(nodes[itmp[2]]);
                    ref_pt[3] = &(nodes[itmp[3]]);
                    getNeighborNodesAB(ref_pt, opp_pts);
                    g = dynamic_cast<Grad3D_ab<T1,NODE>*>(grad3d)->compute(curr_pt, ref_pt, opp_pts, threadNo);
                }
                checkCloseToTx(curr_pt, g, cellNo, Tx, txCell);
                
                std::array<T2,3> ind[4] = {
                    {itmp[0], itmp[1], itmp[2]},
                    {itmp[0], itmp[1], itmp[3]},
                    {itmp[0], itmp[2], itmp[3]},
                    {itmp[1], itmp[2], itmp[3]}};
                for ( size_t n=0; n<4; ++n ) {
                    std::sort( ind[n].begin(), ind[n].end() );
                }
                
                bool foundIntersection = false;
                for ( size_t n=0; n<4; ++n ) {
                    
                    sxyz<T1> pt_i;
                    foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                             ind[n][1], ind[n][2],
                                                             pt_i);
                    
                    if ( !foundIntersection )
                        continue;
                    
                    curr_pt = pt_i;
                    
                    bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                        nodeNo, onEdge, edgeNodes,
                                                        onFace, faceNodes);
                    
                    cell.i = cellNo;
                    cell.v = prev_pt.getDistance( curr_pt );
                    l_data.push_back(cell);
                    tt += slowness[cellNo] * cell.v;
                    prev_pt = curr_pt;
                    
                    if ( break_flag ) break;
                    
                    // find next cell
                    cellNo = findAdjacentCell2(faceNodes, cellNo);
                    if ( cellNo == std::numeric_limits<T2>::max() ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Rx "
                        << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                        l_data.resize(0);
                        reachedTx = true;
                    }
                    break;
                }
                if ( foundIntersection == false ) {
                    std::cout << "\n\nWarning: finding raypath within cell failed to converge for Rx "
                    << Rx.x << ' ' << Rx.y << ' ' << Rx.z << std::endl;
                    l_data.resize(0);
                    reachedTx = true;
                }
            }
            
            if ( onNode ) {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        if ( curr_pt.getDistance( Tx[nt] ) < minDist ) {
                            tt += t0[nt];
                            reachedTx = true;
                            break;
                        }
                    } else if ( txOnEdge[nt] ) {
                        if ( curr_pt.getDistance(nodes[txEdges[nt][0]]) < minDist ||
                            curr_pt.getDistance(nodes[txEdges[nt][1]]) < minDist ) {
                            
                            cell.i = txCell[nt];
                            cell.v = prev_pt.getDistance( Tx[nt] );
                            l_data.push_back(cell);
                            tt += t0[nt] + slowness[txCell[nt]] * cell.v;
                            reachedTx = true;
                            break;
                        }
                    } else if ( txOnFace[nt] ) {
                        if ( curr_pt.getDistance(nodes[txFaces[nt][0]]) < minDist ||
                            curr_pt.getDistance(nodes[txFaces[nt][1]]) < minDist ||
                            curr_pt.getDistance(nodes[txFaces[nt][2]]) < minDist ) {
                            
                            cell.i = txCell[nt];
                            cell.v = prev_pt.getDistance( Tx[nt] );
                            l_data.push_back(cell);
                            tt += t0[nt] + slowness[txCell[nt]] * cell.v;
                            reachedTx = true;
                            break;
                        }
                    }
                }
            } if ( onEdge ) {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        if ( txNode[nt] == edgeNodes[0] || txNode[nt] == edgeNodes[1] ) {
                            cell.i = cellNo;
                            cell.v = prev_pt.getDistance( Tx[nt] );
                            l_data.push_back(cell);
                            tt += t0[nt] + slowness[cellNo] * cell.v;
                            reachedTx = true;
                            break;
                        }
                    } else {
                        std::array<T2,4> itmp = getPrimary(txCell[nt]);
                        // find adjacent cells
                        const T2 ind[6][2] = {
                            {itmp[0], itmp[1]},
                            {itmp[0], itmp[2]},
                            {itmp[0], itmp[3]},
                            {itmp[1], itmp[2]},
                            {itmp[1], itmp[3]},
                            {itmp[2], itmp[3]} };
                        for ( size_t ne=0; ne<6; ++ne ) {
                            if ( (ind[ne][0] == edgeNodes[0] && ind[ne][1] == edgeNodes[1]) ||
                                (ind[ne][0] == edgeNodes[1] && ind[ne][1] == edgeNodes[0]) ) {
                                
                                cell.i = txCell[nt];
                                cell.v = prev_pt.getDistance( Tx[nt] );
                                l_data.push_back(cell);
                                tt += t0[nt] + slowness[txCell[nt]] * cell.v;
                                reachedTx = true;
                                break;
                            }
                        }
                    }
                }
            } else {
                for ( size_t nt=0; nt<Tx.size(); ++nt ) {
                    if ( txOnNode[nt] ) {
                        for ( auto nc=nodes[txNode[nt]].getOwners().begin(); nc!=nodes[txNode[nt]].getOwners().end(); ++nc ) {
                            if ( cellNo == *nc ) {
                                cell.i = cellNo;
                                cell.v = prev_pt.getDistance( Tx[nt] );
                                l_data.push_back(cell);
                                tt += t0[nt] + slowness[cellNo] * cell.v;
                                reachedTx = true;
                                break;
                            }
                        }
                    } else {
                        if ( cellNo == txCell[nt] ) {
                            cell.i = txCell[nt];
                            cell.v = prev_pt.getDistance( Tx[nt] );
                            l_data.push_back(cell);
                            tt += t0[nt] + slowness[cellNo] * cell.v;
                            reachedTx = true;
                        } else {
                            for ( size_t nn=0; nn<txNeighborCells[nt].size(); ++nn ) {
                                if ( cellNo == txNeighborCells[nt][nn] ) {
                                    std::array<T2,4> itmp = getPrimary(txCell[nt]);
                                    std::array<T2,3> ind[4] = {
                                        { { itmp[0], itmp[1], itmp[2] } },
                                        { { itmp[0], itmp[1], itmp[3] } },
                                        { { itmp[0], itmp[2], itmp[3] } },
                                        { { itmp[1], itmp[2], itmp[3] } }
                                    };
                                    
                                    bool found = false;
                                    for ( size_t n=0; n<4; ++n ) {
                                        std::sort( ind[n].begin(), ind[n].end() );
                                        if ( faceNodes == ind[n] ) {
                                            found = true;
                                            break;
                                        }
                                    }
                                    if ( found ) {
                                        cell.i = cellNo;
                                        cell.v = prev_pt.getDistance( Tx[nt] );
                                        l_data.push_back(cell);
                                        tt += t0[nt] + slowness[cellNo] * cell.v;
                                        reachedTx = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if ( reachedTx ) break;
                }
            }
        }
        // must be sorted to build matrix
        sort(l_data.begin(), l_data.end(), CompareSiv_i<T1>());

        delete grad3d;
    }


    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::getStraightRays(const std::vector<sxyz<T1>>& Tx,
                                               const std::vector<sxyz<T1>>& Rx,
                                               std::vector<std::vector<siv<T1>>>& l_data) const {
        T1 minDist = small;
        // Tx & Rx should have the same number of elements, corresponding to
        // Tx-Rx pairs

        l_data.resize(Tx.size());

        // loop over pairs
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {

            if ( Rx[nt] == Tx[nt] ) {
                continue;
            }

            bool txOnNode = false;
            bool txOnEdge = false;
            bool txOnFace = false;
            T2 txNode=0;
            T2 txCell=0;
            std::array<T2,2> txEdges;
            std::array<T2,3> txFaces;
            std::vector<T2> txNeighborCells;

            for ( T2 nn=0; nn<nodes.size(); ++nn ) {
                if ( nodes[nn].isPrimary() ) {
                    if ( nodes[nn] == Tx[nt] ) {
                        txOnNode = true;
                        txNode = nn;
                        break;
                    }
                }
            }
            if ( !txOnNode ) {
                txCell = getCellNo( Tx[nt] );

                std::array<T2,4> itmp = getPrimary(txCell);
                // find adjacent cells
                const T2 ind[6][2] = {
                    {itmp[0], itmp[1]},
                    {itmp[0], itmp[2]},
                    {itmp[0], itmp[3]},
                    {itmp[1], itmp[2]},
                    {itmp[1], itmp[3]},
                    {itmp[2], itmp[3]} };

                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    for ( auto nc0=nodes[ind[nedge][0]].getOwners().begin(); nc0!=nodes[ind[nedge][0]].getOwners().end(); ++nc0 ) {
                        if ( std::find(nodes[ind[nedge][1]].getOwners().begin(), nodes[ind[nedge][1]].getOwners().end(), *nc0)!=nodes[ind[nedge][1]].getOwners().end() )
                            txNeighborCells.push_back( *nc0 );
                    }
                }
                // check if on edge
                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    if ( distSqPointToSegment( &nodes[ind[nedge][0]], &nodes[ind[nedge][1]], Tx[nt]) < small2 ) {
                        txOnEdge = true;
                        txEdges[0] = ind[nedge][0];
                        txEdges[1] = ind[nedge][1];
                        break;
                    }
                }
                if ( !txOnEdge ) {
                    // check if on face
                    const T2 indf[4][3] = {
                        {itmp[0], itmp[1], itmp[2]},
                        {itmp[0], itmp[1], itmp[3]},
                        {itmp[0], itmp[2], itmp[3]},
                        {itmp[1], itmp[2], itmp[3]} };
                    for ( size_t nface=0; nface<4; ++nface ) {
                        if ( testInTriangle(&nodes[indf[nface][0]], &nodes[indf[nface][1]], &nodes[indf[nface][2]], Tx[nt]) ) {
                            txOnFace = true;
                            txFaces[0] = indf[nface][0];
                            txFaces[1] = indf[nface][1];
                            txFaces[2] = indf[nface][2];
                            break;
                        }
                    }
                }
            }
            T2 cellNo=0, nodeNo=0;
            sxyz<T1> curr_pt( Rx[nt] ), prev_pt( Rx[nt] );

            bool onNode = false;
            bool onEdge = false;
            bool onFace = false;
            std::array<T2,2> edgeNodes{ {0, 0} };
            std::array<T2,3> faceNodes{ {0, 0, 0} };
            std::array<T2,3> faceNodesStart{ {0, 0, 0} };
            bool reachedTx = false;

            for ( T2 nn=0; nn<nodes.size(); ++nn ) {
                if ( nodes[nn] == curr_pt ) {
                    nodeNo = nn;
                    onNode = true;
                    break;
                }
            }
            if ( !onNode ) {
                cellNo = getCellNo( curr_pt );

                std::array<T2,4> itmp = getPrimary(cellNo);
                T2 ind[6][2] = {
                    {itmp[0], itmp[1]},
                    {itmp[0], itmp[2]},
                    {itmp[0], itmp[3]},
                    {itmp[1], itmp[2]},
                    {itmp[1], itmp[3]},
                    {itmp[2], itmp[3]} };

                for ( size_t n=0; n<6; ++n ) {
                    if ( areCollinear(curr_pt, nodes[ind[n][0]], nodes[ind[n][1]]) ) {
                        onEdge = true;
                        edgeNodes[0] = ind[n][0];
                        edgeNodes[1] = ind[n][1];
                        break;
                    }
                }
                if ( !onEdge ) {
                    std::array<T2,4> itmp = getPrimary(cellNo);
                    std::array<T2,3> ind[4] = {
                        { { itmp[0], itmp[1], itmp[2] } },
                        { { itmp[0], itmp[1], itmp[3] } },
                        { { itmp[0], itmp[2], itmp[3] } },
                        { { itmp[1], itmp[2], itmp[3] } }
                    };
                    for ( size_t n=0; n<4; ++n ) {
                        std::sort( ind[n].begin(), ind[n].end() );
                    }

                    for ( size_t n=0; n<4; ++n ) {
                        if ( areCoplanar(curr_pt, nodes[ind[n][0]], nodes[ind[n][1]], nodes[ind[n][2]]) ) {
                            onFace = true;
                            //  faceNodes shoud not be assigned, face was not intersected
                            break;
                        }
                    }
                }
            }

            siv<T1> cell;
            if ( cellNo == txCell ) {
                cell.i = txCell;
                cell.v = curr_pt.getDistance(Tx[nt]);
                l_data[nt].push_back( cell );
                reachedTx = true;
                break;
            }

            sxyz<T1> g = Tx[nt] - Rx[nt];
            while ( reachedTx == false ) {
                if ( onNode ) {
                    // find cell for which gradient intersect opposing face
                    bool foundIntersection = false;
                    for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {

                        std::array<T2,3> nb;
                        size_t n=0;
                        for (auto nn=this->neighbors[*nc].begin(); nn!=this->neighbors[*nc].end(); ++nn ) {
                            if ( *nn != nodeNo && nodes[*nn].isPrimary() ) {
                                nb[n++] = *nn;
                            }
                        }
                        std::sort(nb.begin(), nb.end());

                        foundIntersection = intersectVecTriangle( nodeNo, g, nb[0], nb[1], nb[2], curr_pt);
                        if ( !foundIntersection ) {
                            continue;
                        }

                        //  check if cell is TxCell
                        if ( *nc == txCell ) {
                            cell.i = txCell;
                            cell.v = curr_pt.getDistance(Tx[nt]);
                            l_data[nt].push_back( cell );
                            reachedTx = true;
                            break;
                        }
                        if ( reachedTx ) {
                            break;
                        }

                        bool break_flag = check_pt_location(curr_pt, nb, onNode,
                                                            nodeNo, onEdge, edgeNodes,
                                                            onFace, faceNodes);

                        sxyz<T1> mid_pt = static_cast<T1>(0.5)*(prev_pt + curr_pt);
                        cell.i = getCellNo(mid_pt);
                        cell.v = curr_pt.getDistance(prev_pt);
                        l_data[nt].push_back(cell);
                        prev_pt = curr_pt;

                        if ( break_flag ) break;

                        // find next cell
                        cellNo = findAdjacentCell1(faceNodes, nodeNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            l_data[nt].resize(0);
                            reachedTx = true;
                        }
                        break;
                    }

                    if ( foundIntersection == false ) {
                        // projet gradient on face and find intersection on next edge
                        sxyz<T1> pt_i;
                        foundIntersection = projectOnFace(curr_pt, nodeNo, g, edgeNodes, pt_i);

                        if ( foundIntersection == false ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            l_data[nt].resize(0);
                            reachedTx = true;
                            break;
                        }

                        // find which cell we are in
                        for ( size_t nc=0; nc<tetrahedra.size(); ++nc ) {
                            std::array<T2,4> itet = {tetrahedra[nc].i[0],
                                tetrahedra[nc].i[1],
                                tetrahedra[nc].i[2],
                                tetrahedra[nc].i[3]};
                            // because we are at the surface, there is only one cell with nodeNo & edgeNodes
                            if (std::find(itet.begin(), itet.end(), nodeNo) != itet.end() &&
                                std::find(itet.begin(), itet.end(), edgeNodes[0]) != itet.end() &&
                                std::find(itet.begin(), itet.end(), edgeNodes[1]) != itet.end()) {
                                cellNo = static_cast<T2>(nc);
                                break;
                            }
                        }

                        //  check if cell is TxCell
                        if ( cellNo == txCell ) {
                            cell.i = txCell;
                            cell.v = curr_pt.getDistance(Tx[nt]);
                            l_data[nt].push_back( cell );
                            reachedTx = true;
                            break;
                        }
                        if ( reachedTx ) {
                            break;
                        }
                        // we might be on one of the nodes
                        if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                            onNode = true;
                            nodeNo = edgeNodes[0];
                            pt_i = nodes[edgeNodes[0]];
                            onEdge = false;
                        } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                            onNode = true;
                            nodeNo = edgeNodes[1];
                            pt_i = nodes[edgeNodes[1]];
                            onEdge = false;
                        } else {
                            onEdge = true;
                            onNode = false;
                        }

                        curr_pt = pt_i;
                        sxyz<T1> mid_pt = static_cast<T1>(0.5)*(prev_pt + curr_pt);
                        cell.i = getCellNo(mid_pt);
                        cell.v = curr_pt.getDistance(prev_pt);
                        l_data[nt].push_back(cell);
                        prev_pt = curr_pt;
                    }

                } else if ( onEdge ) {

                    // find cells common to edge
                    std::vector<T2> cells;
                    for ( auto nc0=nodes[edgeNodes[0]].getOwners().begin(); nc0!=nodes[edgeNodes[0]].getOwners().end(); ++nc0 ) {
                        if ( std::find(nodes[edgeNodes[1]].getOwners().begin(), nodes[edgeNodes[1]].getOwners().end(), *nc0)!=nodes[edgeNodes[1]].getOwners().end() ) {
                            cells.push_back( *nc0 );
                        }
                    }

                    bool foundIntersection=false;
                    for (size_t n=0; n<cells.size(); ++n ) {

                        T2 testCellNo = cells[n];

                        // there are 2 faces that might be intersected
                        std::array<T2,2> edgeNodes2;
                        size_t n2=0;
                        for ( auto nn=this->neighbors[testCellNo].begin(); nn!= this->neighbors[testCellNo].end(); ++nn ) {
                            if ( *nn!=edgeNodes[0] && *nn!=edgeNodes[1] && nodes[*nn].isPrimary() ) {
                                edgeNodes2[n2++] = *nn;
                            }
                        }

                        sxyz<T1> pt_i;
                        T2 itmpNode;
                        foundIntersection = intersectVecTriangle(curr_pt, g,
                                                                 edgeNodes[0],
                                                                 edgeNodes2[0],
                                                                 edgeNodes2[1], pt_i);
                        itmpNode = edgeNodes[0];
                        if ( !foundIntersection ) {
                            foundIntersection = intersectVecTriangle(curr_pt, g,
                                                                     edgeNodes[1],
                                                                     edgeNodes2[0],
                                                                     edgeNodes2[1], pt_i);
                            itmpNode = edgeNodes[1];
                        }
                        if ( !foundIntersection ) {
                            continue;
                        }

                        cellNo = testCellNo;
                        curr_pt = pt_i;

                        bool break_flag = check_pt_location(curr_pt, this->neighbors[cellNo],
                                                            {itmpNode, edgeNodes2[0], edgeNodes2[1]},
                                                            onNode,
                                                            nodeNo, onEdge, edgeNodes,
                                                            onFace, faceNodes);

                        sxyz<T1> mid_pt = static_cast<T1>(0.5)*(prev_pt + curr_pt);
                        cell.i = getCellNo(mid_pt);
                        cell.v = curr_pt.getDistance(prev_pt);
                        l_data[nt].push_back(cell);
                        prev_pt = curr_pt;

                        if ( break_flag ) break;

                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            l_data[nt].resize(0);
                            reachedTx = true;
                        }
                        break;
                    }
                    if ( foundIntersection == false ) {
                        sxyz<T1> pt_i;
                        g = projectOnFace(curr_pt, g, edgeNodes, cells, pt_i);
                        if ( g.x==0.0 && g.y==0.0 && g.z==0.0 ) {
                            foundIntersection = false;
                        } else {
                            foundIntersection = true;
                        }

                        if ( foundIntersection == false || curr_pt == pt_i ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            l_data[nt].resize(0);
                            reachedTx = true;
                        }

                        // we might be on one of the nodes
                        if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                            onNode = true;
                            nodeNo = edgeNodes[0];
                            pt_i = nodes[edgeNodes[0]];
                            onEdge = false;
                        } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                            onNode = true;
                            nodeNo = edgeNodes[1];
                            pt_i = nodes[edgeNodes[1]];
                            onEdge = false;
                        }

                        curr_pt = pt_i;
                        sxyz<T1> mid_pt = static_cast<T1>(0.5)*(prev_pt + curr_pt);
                        cell.i = getCellNo(mid_pt);
                        cell.v = curr_pt.getDistance(prev_pt);
                        l_data[nt].push_back(cell);
                        prev_pt = curr_pt;
                    }

                } else if ( onFace ) {

                    std::array<T2,4> itmp = getPrimary(cellNo);

                    std::array<T2,3> ind[4] = {
                        {itmp[0], itmp[1], itmp[2]},
                        {itmp[0], itmp[1], itmp[3]},
                        {itmp[0], itmp[2], itmp[3]},
                        {itmp[1], itmp[2], itmp[3]}
                    };
                    for ( size_t n=0; n<4; ++n ) {
                        std::sort( ind[n].begin(), ind[n].end() );
                    }
                    // there are 3 faces that might be intersected

                    bool foundIntersection = false;
                    for ( size_t n=0; n<4; ++n ) {
                        if ( ind[n] == faceNodes ) continue;

                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                 ind[n][1], ind[n][2],
                                                                 pt_i);

                        if ( !foundIntersection )
                            continue;

                        curr_pt = pt_i;

                        bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                            nodeNo, onEdge, edgeNodes,
                                                            onFace, faceNodes);

                        sxyz<T1> mid_pt = static_cast<T1>(0.5)*(prev_pt + curr_pt);
                        cell.i = getCellNo(mid_pt);
                        cell.v = curr_pt.getDistance(prev_pt);
                        l_data[nt].push_back(cell);
                        prev_pt = curr_pt;

                        if ( break_flag ) break;

                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            l_data[nt].resize(0);
                            reachedTx = true;
                        }
                        break;
                    }

                    if ( foundIntersection == false ) {

                        if ( (faceNodes[0] == faceNodes[1]) && (faceNodes[0] == faceNodes[2]) ) {
                            // we have started on a face and cellNo was incorrectly picked
                            // find adjacent cell
                            cellNo = findAdjacentCell2(faceNodesStart, cellNo);
                        } else {
                            // we must be on an face with gradient pointing slightly outward tetrahedron
                            // return in other cell but keep gradient
                            cellNo = findAdjacentCell2(faceNodes, cellNo);
                        }

                        std::array<T2,4> itmp = getPrimary(cellNo);
                        ind[0] = {itmp[0], itmp[1], itmp[2]};
                        ind[1] = {itmp[0], itmp[1], itmp[3]};
                        ind[2] = {itmp[0], itmp[2], itmp[3]};
                        ind[3] = {itmp[1], itmp[2], itmp[3]};

                        for ( size_t n=0; n<4; ++n ) {
                            std::sort( ind[n].begin(), ind[n].end() );
                        }

                        for ( size_t n=0; n<4; ++n ) {
                            if ( ind[n] == faceNodes ) continue;

                            sxyz<T1> pt_i;
                            foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                     ind[n][1], ind[n][2],
                                                                     pt_i);

                            if ( !foundIntersection ) {
                                continue;
                            }
                            curr_pt = pt_i;

                            bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                                nodeNo, onEdge, edgeNodes,
                                                                onFace, faceNodes);

                            sxyz<T1> mid_pt = static_cast<T1>(0.5)*(prev_pt + curr_pt);
                            cell.i = getCellNo(mid_pt);
                            cell.v = curr_pt.getDistance(prev_pt);
                            l_data[nt].push_back(cell);
                            prev_pt = curr_pt;

                            if ( break_flag ) break;

                            // find next cell
                            cellNo = findAdjacentCell2(faceNodes, cellNo);
                            if ( cellNo == std::numeric_limits<T2>::max() ) {
                                std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                                << nt << std::endl;
                                l_data[nt].resize(0);
                                reachedTx = true;
                            }
                            break;
                        }
                    }
                    if ( foundIntersection == false ) {
                        g = projectOnFace(g, faceNodes);

                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecEdge(curr_pt, g, faceNodes, pt_i, edgeNodes);

                        if ( foundIntersection == false ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            l_data[nt].resize(0);
                            reachedTx = true;
                            break;
                        }

                        // we might be on one of the nodes
                        if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                            onNode = true;
                            nodeNo = edgeNodes[0];
                            pt_i = nodes[edgeNodes[0]];
                            onEdge = false;
                            onFace = false;
                        } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                            onNode = true;
                            nodeNo = edgeNodes[1];
                            pt_i = nodes[edgeNodes[1]];
                            onEdge = false;
                            onFace = false;
                        } else {
                            onEdge = true;
                            onFace = false;
                        }
                        curr_pt = pt_i;
                        sxyz<T1> mid_pt = static_cast<T1>(0.5)*(prev_pt + curr_pt);
                        cell.i = getCellNo(mid_pt);
                        cell.v = curr_pt.getDistance(prev_pt);
                        l_data[nt].push_back(cell);
                        prev_pt = curr_pt;
                    }
                } else { // at Rx, somewhere in a tetrahedron

                    std::array<T2,4> itmp = getPrimary(cellNo);

                    std::array<T2,3> ind[4] = {
                        {itmp[0], itmp[1], itmp[2]},
                        {itmp[0], itmp[1], itmp[3]},
                        {itmp[0], itmp[2], itmp[3]},
                        {itmp[1], itmp[2], itmp[3]}

                    };
                    for ( size_t n=0; n<4; ++n ) {
                        std::sort( ind[n].begin(), ind[n].end() );
                    }

                    bool foundIntersection = false;
                    for ( size_t n=0; n<4; ++n ) {

                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                 ind[n][1], ind[n][2],
                                                                 pt_i);

                        if ( !foundIntersection )
                            continue;

                        curr_pt = pt_i;

                        bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                            nodeNo, onEdge, edgeNodes,
                                                            onFace, faceNodes);

                        sxyz<T1> mid_pt = static_cast<T1>(0.5)*(prev_pt + curr_pt);
                        cell.i = getCellNo(mid_pt);
                        cell.v = curr_pt.getDistance(prev_pt);
                        l_data[nt].push_back(cell);
                        prev_pt = curr_pt;

                        if ( break_flag ) break;

                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            l_data[nt].resize(0);
                            reachedTx = true;
                        }
                        break;
                    }
                    if ( foundIntersection == false ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                        << nt << std::endl;
                        l_data[nt].resize(0);
                        reachedTx = true;
                    }
                }

                if ( onNode ) {
                    if ( txOnNode ) {
                        if ( curr_pt.getDistance( Tx[nt] ) < minDist ) {
                            reachedTx = true;
                            break;
                        }
                    } else if ( txOnEdge ) {
                        if ( curr_pt.getDistance(nodes[txEdges[0]]) < minDist ||
                            curr_pt.getDistance(nodes[txEdges[1]]) < minDist ) {

                            sxyz<T1> mid_pt = static_cast<T1>(0.5)*(prev_pt + Tx[nt]);
                            cell.i = getCellNo(mid_pt);
                            cell.v = curr_pt.getDistance(prev_pt);
                            l_data[nt].push_back(cell);
                            reachedTx = true;
                            break;
                        }
                    } else if ( txOnFace ) {
                        if ( curr_pt.getDistance(nodes[txFaces[0]]) < minDist ||
                            curr_pt.getDistance(nodes[txFaces[1]]) < minDist ||
                            curr_pt.getDistance(nodes[txFaces[2]]) < minDist ) {

                            sxyz<T1> mid_pt = static_cast<T1>(0.5)*(prev_pt + Tx[nt]);
                            cell.i = getCellNo(mid_pt);
                            cell.v = curr_pt.getDistance(prev_pt);
                            l_data[nt].push_back(cell);
                            reachedTx = true;
                            break;
                        }
                    }
                } if ( onEdge ) {
                    if ( txOnNode ) {
                        if ( txNode == edgeNodes[0] || txNode == edgeNodes[1] ) {
                            sxyz<T1> mid_pt = static_cast<T1>(0.5)*(prev_pt + Tx[nt]);
                            cell.i = getCellNo(mid_pt);
                            cell.v = curr_pt.getDistance(prev_pt);
                            l_data[nt].push_back(cell);
                            reachedTx = true;
                            break;
                        }
                    } else {
                        std::array<T2,4> itmp = getPrimary(txCell);
                        // find adjacent cells
                        const T2 ind[6][2] = {
                            {itmp[0], itmp[1]},
                            {itmp[0], itmp[2]},
                            {itmp[0], itmp[3]},
                            {itmp[1], itmp[2]},
                            {itmp[1], itmp[3]},
                            {itmp[2], itmp[3]} };
                        for ( size_t ne=0; ne<6; ++ne ) {
                            if ( (ind[ne][0] == edgeNodes[0] && ind[ne][1] == edgeNodes[1]) ||
                                (ind[ne][0] == edgeNodes[1] && ind[ne][1] == edgeNodes[0]) ) {
                                sxyz<T1> mid_pt = static_cast<T1>(0.5)*(prev_pt + Tx[nt]);
                                cell.i = getCellNo(mid_pt);
                                cell.v = curr_pt.getDistance(prev_pt);
                                l_data[nt].push_back(cell);
                                reachedTx = true;
                                break;
                            }
                        }
                    }
                } else {
                    if ( txOnNode ) {
                        for ( auto nc=nodes[txNode].getOwners().begin(); nc!=nodes[txNode].getOwners().end(); ++nc ) {
                            if ( cellNo == *nc ) {
                                cell.i = cellNo;
                                cell.v = curr_pt.getDistance(Tx[nt]);
                                l_data[nt].push_back( cell );
                                reachedTx = true;
                                break;
                            }
                        }
                    } else {
                        if ( cellNo == txCell ) {
                            cell.i = txCell;
                            cell.v = curr_pt.getDistance(Tx[nt]);
                            l_data[nt].push_back( cell );
                            reachedTx = true;
                        } else {
                            for ( size_t nn=0; nn<txNeighborCells.size(); ++nn ) {
                                if ( cellNo == txNeighborCells[nn] ) {
                                    std::array<T2,4> itmp = getPrimary(txCell);
                                    std::array<T2,3> ind[4] = {
                                        { { itmp[0], itmp[1], itmp[2] } },
                                        { { itmp[0], itmp[1], itmp[3] } },
                                        { { itmp[0], itmp[2], itmp[3] } },
                                        { { itmp[1], itmp[2], itmp[3] } }
                                    };

                                    bool found = false;
                                    for ( size_t n=0; n<4; ++n ) {
                                        std::sort( ind[n].begin(), ind[n].end() );
                                        if ( faceNodes == ind[n] ) {
                                            found = true;
                                            break;
                                        }
                                    }
                                    if ( found ) {
                                        cell.i = cellNo;
                                        cell.v = curr_pt.getDistance(Tx[nt]);
                                        l_data[nt].push_back( cell );
                                        reachedTx = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if ( reachedTx ) break;
                }
            }
        }
    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::getStraightRays(const std::vector<sxyz<T1>>& Tx,
                                               const std::vector<sxyz<T1>>& Rx,
                                               std::vector<std::vector<sxyz<T1>>>& r_data) const {
        T1 minDist = small;
        // Tx & Rx should have the same number of elements, corresponding to
        // Tx-Rx pairs
        r_data.resize(Tx.size());
        // loop over pairs
        for ( size_t nt=0; nt<Tx.size(); ++nt ) {
            r_data[nt].emplace_back( Rx[nt] );

            if ( Rx[nt] == Tx[nt] ) {
                continue;
            }

            bool txOnNode = false;
            bool txOnEdge = false;
            bool txOnFace = false;
            T2 txNode;
            T2 txCell;
            std::array<T2,2> txEdges;
            std::array<T2,3> txFaces;
            std::vector<T2> txNeighborCells;

            for ( T2 nn=0; nn<nodes.size(); ++nn ) {
                if ( nodes[nn].isPrimary() ) {
                    if ( nodes[nn] == Tx[nt] ) {
                        txOnNode = true;
                        txNode = nn;
                        break;
                    }
                }
            }
            if ( !txOnNode ) {
                txCell = getCellNo( Tx[nt] );

                std::array<T2,4> itmp = getPrimary(txCell);
                // find adjacent cells
                const T2 ind[6][2] = {
                    {itmp[0], itmp[1]},
                    {itmp[0], itmp[2]},
                    {itmp[0], itmp[3]},
                    {itmp[1], itmp[2]},
                    {itmp[1], itmp[3]},
                    {itmp[2], itmp[3]} };

                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    for ( auto nc0=nodes[ind[nedge][0]].getOwners().begin(); nc0!=nodes[ind[nedge][0]].getOwners().end(); ++nc0 ) {
                        if ( std::find(nodes[ind[nedge][1]].getOwners().begin(), nodes[ind[nedge][1]].getOwners().end(), *nc0)!=nodes[ind[nedge][1]].getOwners().end() )
                            txNeighborCells.push_back( *nc0 );
                    }
                }
                // check if on edge
                for ( size_t nedge=0; nedge<6; ++nedge ) {
                    if ( distSqPointToSegment( &nodes[ind[nedge][0]], &nodes[ind[nedge][1]], Tx[nt]) < small2 ) {
                        txOnEdge = true;
                        txEdges[0] = ind[nedge][0];
                        txEdges[1] = ind[nedge][1];
                        break;
                    }
                }
                if ( !txOnEdge ) {
                    // check if on face
                    const T2 indf[4][3] = {
                        {itmp[0], itmp[1], itmp[2]},
                        {itmp[0], itmp[1], itmp[3]},
                        {itmp[0], itmp[2], itmp[3]},
                        {itmp[1], itmp[2], itmp[3]} };
                    for ( size_t nface=0; nface<4; ++nface ) {
                        if ( testInTriangle(&nodes[indf[nface][0]], &nodes[indf[nface][1]], &nodes[indf[nface][2]], Tx[nt]) ) {
                            txOnFace = true;
                            txFaces[0] = indf[nface][0];
                            txFaces[1] = indf[nface][1];
                            txFaces[2] = indf[nface][2];
                            break;
                        }
                    }
                }
            }
            T2 cellNo, nodeNo;
            sxyz<T1> curr_pt( Rx[nt] );

            bool onNode = false;
            bool onEdge = false;
            bool onFace = false;
            std::array<T2,2> edgeNodes{ {0, 0} };
            std::array<T2,3> faceNodes{ {0, 0, 0} };
            std::array<T2,3> faceNodesStart{ {0, 0, 0} };
            bool reachedTx = false;

            for ( T2 nn=0; nn<nodes.size(); ++nn ) {
                if ( nodes[nn] == curr_pt ) {
                    nodeNo = nn;
                    onNode = true;
                    break;
                }
            }
            if ( !onNode ) {
                cellNo = getCellNo( curr_pt );

                std::array<T2,4> itmp = getPrimary(cellNo);
                T2 ind[6][2] = {
                    {itmp[0], itmp[1]},
                    {itmp[0], itmp[2]},
                    {itmp[0], itmp[3]},
                    {itmp[1], itmp[2]},
                    {itmp[1], itmp[3]},
                    {itmp[2], itmp[3]} };

                for ( size_t n=0; n<6; ++n ) {
                    if ( areCollinear(curr_pt, nodes[ind[n][0]], nodes[ind[n][1]]) ) {
                        onEdge = true;
                        edgeNodes[0] = ind[n][0];
                        edgeNodes[1] = ind[n][1];
                        break;
                    }
                }
                if ( !onEdge ) {
                    std::array<T2,4> itmp = getPrimary(cellNo);
                    std::array<T2,3> ind[4] = {
                        { { itmp[0], itmp[1], itmp[2] } },
                        { { itmp[0], itmp[1], itmp[3] } },
                        { { itmp[0], itmp[2], itmp[3] } },
                        { { itmp[1], itmp[2], itmp[3] } }
                    };
                    for ( size_t n=0; n<4; ++n ) {
                        std::sort( ind[n].begin(), ind[n].end() );
                    }

                    for ( size_t n=0; n<4; ++n ) {
                        if ( areCoplanar(curr_pt, nodes[ind[n][0]], nodes[ind[n][1]], nodes[ind[n][2]]) ) {
                            onFace = true;
                            //  faceNodes shoud not be assigned, face was not intersected
                            break;
                        }
                    }
                }
            }

            if ( cellNo == txCell ) {
                r_data[nt].push_back( Tx[nt] );
                reachedTx = true;
                break;
            }

            sxyz<T1> g = Tx[nt] - Rx[nt];
            while ( reachedTx == false ) {
                if ( onNode ) {
                    // find cell for which gradient intersect opposing face
                    bool foundIntersection = false;
                    for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {

                        std::array<T2,3> nb;
                        size_t n=0;
                        for (auto nn=this->neighbors[*nc].begin(); nn!=this->neighbors[*nc].end(); ++nn ) {
                            if ( *nn != nodeNo && nodes[*nn].isPrimary() ) {
                                nb[n++] = *nn;
                            }
                        }
                        std::sort(nb.begin(), nb.end());

                        foundIntersection = intersectVecTriangle( nodeNo, g, nb[0], nb[1], nb[2], curr_pt);
                        if ( !foundIntersection ) {
                            continue;
                        }

                        //  check if cell is TxCell
                        if ( *nc == txCell ) {
                            r_data[nt].push_back( Tx[nt] );
                            reachedTx = true;
                            break;
                        }
                        if ( reachedTx ) {
                            break;
                        }

                        bool break_flag = check_pt_location(curr_pt, nb, onNode,
                                                            nodeNo, onEdge, edgeNodes,
                                                            onFace, faceNodes);

                        r_data[nt].push_back( curr_pt );

                        if ( break_flag ) break;

                        // find next cell
                        cellNo = findAdjacentCell1(faceNodes, nodeNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            r_data[nt].resize(1);
                            r_data[nt][0] = Rx[nt];
                            reachedTx = true;
                        }
                        break;
                    }

                    if ( foundIntersection == false ) {
                        // projet gradient on face and find intersection on next edge
                        sxyz<T1> pt_i;
                        foundIntersection = projectOnFace(curr_pt, nodeNo, g, edgeNodes, pt_i);

                        if ( foundIntersection == false ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            r_data[nt].resize(1);
                            r_data[nt][0] = Rx[nt];
                            reachedTx = true;
                            break;
                        }

                        // find which cell we are in
                        for ( size_t nc=0; nc<tetrahedra.size(); ++nc ) {
                            std::array<T2,4> itet = {tetrahedra[nc].i[0],
                                tetrahedra[nc].i[1],
                                tetrahedra[nc].i[2],
                                tetrahedra[nc].i[3]};
                            // because we are at the surface, there is only one cell with nodeNo & edgeNodes
                            if (std::find(itet.begin(), itet.end(), nodeNo) != itet.end() &&
                                std::find(itet.begin(), itet.end(), edgeNodes[0]) != itet.end() &&
                                std::find(itet.begin(), itet.end(), edgeNodes[1]) != itet.end()) {
                                cellNo = static_cast<T2>(nc);
                                break;
                            }
                        }

                        //  check if cell is TxCell
                        if ( cellNo == txCell ) {
                            r_data[nt].push_back( Tx[nt] );
                            reachedTx = true;
                            break;
                        }
                        if ( reachedTx ) {
                            break;
                        }
                        // we might be on one of the nodes
                        if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                            onNode = true;
                            nodeNo = edgeNodes[0];
                            pt_i = nodes[edgeNodes[0]];
                            onEdge = false;
                        } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                            onNode = true;
                            nodeNo = edgeNodes[1];
                            pt_i = nodes[edgeNodes[1]];
                            onEdge = false;
                        } else {
                            onEdge = true;
                            onNode = false;
                        }

                        curr_pt = pt_i;
                        r_data[nt].push_back( curr_pt );
                    }

                } else if ( onEdge ) {

                    // find cells common to edge
                    std::vector<T2> cells;
                    for ( auto nc0=nodes[edgeNodes[0]].getOwners().begin(); nc0!=nodes[edgeNodes[0]].getOwners().end(); ++nc0 ) {
                        if ( std::find(nodes[edgeNodes[1]].getOwners().begin(), nodes[edgeNodes[1]].getOwners().end(), *nc0)!=nodes[edgeNodes[1]].getOwners().end() ) {
                            cells.push_back( *nc0 );
                        }
                    }

                    bool foundIntersection=false;
                    for (size_t n=0; n<cells.size(); ++n ) {

                        T2 testCellNo = cells[n];

                        // there are 2 faces that might be intersected
                        std::array<T2,2> edgeNodes2;
                        size_t n2=0;
                        for ( auto nn=this->neighbors[testCellNo].begin(); nn!= this->neighbors[testCellNo].end(); ++nn ) {
                            if ( *nn!=edgeNodes[0] && *nn!=edgeNodes[1] && nodes[*nn].isPrimary() ) {
                                edgeNodes2[n2++] = *nn;
                            }
                        }

                        sxyz<T1> pt_i;
                        T2 itmpNode;
                        foundIntersection = intersectVecTriangle(curr_pt, g,
                                                                 edgeNodes[0],
                                                                 edgeNodes2[0],
                                                                 edgeNodes2[1], pt_i);
                        itmpNode = edgeNodes[0];
                        if ( !foundIntersection ) {
                            foundIntersection = intersectVecTriangle(curr_pt, g,
                                                                     edgeNodes[1],
                                                                     edgeNodes2[0],
                                                                     edgeNodes2[1], pt_i);
                            itmpNode = edgeNodes[1];
                        }
                        if ( !foundIntersection ) {
                            continue;
                        }

                        cellNo = testCellNo;
                        curr_pt = pt_i;

                        bool break_flag = check_pt_location(curr_pt, this->neighbors[cellNo],
                                                            {itmpNode, edgeNodes2[0], edgeNodes2[1]},
                                                            onNode,
                                                            nodeNo, onEdge, edgeNodes,
                                                            onFace, faceNodes);

                        r_data[nt].push_back( curr_pt );

                        if ( break_flag ) break;

                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            r_data[nt].resize(1);
                            r_data[nt][0] = Rx[nt];
                            reachedTx = true;
                        }
                        break;
                    }
                    if ( foundIntersection == false ) {
                        sxyz<T1> pt_i;
                        g = projectOnFace(curr_pt, g, edgeNodes, cells, pt_i);
                        if ( g.x==0.0 && g.y==0.0 && g.z==0.0 ) {
                            foundIntersection = false;
                        } else {
                            foundIntersection = true;
                        }

                        if ( foundIntersection == false || curr_pt == pt_i ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            r_data[nt].resize(1);
                            r_data[nt][0] = Rx[nt];
                            reachedTx = true;
                        }

                        // we might be on one of the nodes
                        if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                            onNode = true;
                            nodeNo = edgeNodes[0];
                            pt_i = nodes[edgeNodes[0]];
                            onEdge = false;
                        } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                            onNode = true;
                            nodeNo = edgeNodes[1];
                            pt_i = nodes[edgeNodes[1]];
                            onEdge = false;
                        }

                        curr_pt = pt_i;
                        r_data[nt].push_back(curr_pt);
                    }

                } else if ( onFace ) {

                    std::array<T2,4> itmp = getPrimary(cellNo);

                    std::array<T2,3> ind[4] = {
                        {itmp[0], itmp[1], itmp[2]},
                        {itmp[0], itmp[1], itmp[3]},
                        {itmp[0], itmp[2], itmp[3]},
                        {itmp[1], itmp[2], itmp[3]}
                    };
                    for ( size_t n=0; n<4; ++n ) {
                        std::sort( ind[n].begin(), ind[n].end() );
                    }
                    // there are 3 faces that might be intersected

                    bool foundIntersection = false;
                    for ( size_t n=0; n<4; ++n ) {
                        if ( ind[n] == faceNodes ) continue;

                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                 ind[n][1], ind[n][2],
                                                                 pt_i);

                        if ( !foundIntersection )
                            continue;

                        curr_pt = pt_i;

                        bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                            nodeNo, onEdge, edgeNodes,
                                                            onFace, faceNodes);

                        r_data[nt].push_back( curr_pt );

                        if ( break_flag ) break;

                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            r_data[nt].resize(1);
                            r_data[nt][0] = Rx[nt];
                            reachedTx = true;
                        }
                        break;
                    }

                    if ( foundIntersection == false ) {

                        if ( (faceNodes[0] == faceNodes[1]) && (faceNodes[0] == faceNodes[2]) ) {
                            // we have started on a face and cellNo was incorrectly picked
                            // find adjacent cell
                            cellNo = findAdjacentCell2(faceNodesStart, cellNo);
                        } else {
                            // we must be on an face with gradient pointing slightly outward tetrahedron
                            // return in other cell but keep gradient
                            cellNo = findAdjacentCell2(faceNodes, cellNo);
                        }

                        std::array<T2,4> itmp = getPrimary(cellNo);
                        ind[0] = {itmp[0], itmp[1], itmp[2]};
                        ind[1] = {itmp[0], itmp[1], itmp[3]};
                        ind[2] = {itmp[0], itmp[2], itmp[3]};
                        ind[3] = {itmp[1], itmp[2], itmp[3]};

                        for ( size_t n=0; n<4; ++n ) {
                            std::sort( ind[n].begin(), ind[n].end() );
                        }

                        for ( size_t n=0; n<4; ++n ) {
                            if ( ind[n] == faceNodes ) continue;

                            sxyz<T1> pt_i;
                            foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                     ind[n][1], ind[n][2],
                                                                     pt_i);

                            if ( !foundIntersection ) {
                                continue;
                            }
                            curr_pt = pt_i;

                            bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                                nodeNo, onEdge, edgeNodes,
                                                                onFace, faceNodes);

                            r_data[nt].push_back( curr_pt );

                            if ( break_flag ) break;

                            // find next cell
                            cellNo = findAdjacentCell2(faceNodes, cellNo);
                            if ( cellNo == std::numeric_limits<T2>::max() ) {
                                std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                                << nt << std::endl;
                                r_data[nt].resize(1);
                                r_data[nt][0] = Rx[nt];
                                reachedTx = true;
                            }
                            break;
                        }
                    }
                    if ( foundIntersection == false ) {
                        g = projectOnFace(g, faceNodes);

                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecEdge(curr_pt, g, faceNodes, pt_i, edgeNodes);

                        if ( foundIntersection == false ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            r_data[nt].resize(1);
                            r_data[nt][0] = Rx[nt];
                            reachedTx = true;
                            break;
                        }

                        // we might be on one of the nodes
                        if ( pt_i.getDistance(nodes[edgeNodes[0]]) < min_dist ) {
                            onNode = true;
                            nodeNo = edgeNodes[0];
                            pt_i = nodes[edgeNodes[0]];
                            onEdge = false;
                            onFace = false;
                        } else if ( pt_i.getDistance(nodes[edgeNodes[1]]) < min_dist ) {
                            onNode = true;
                            nodeNo = edgeNodes[1];
                            pt_i = nodes[edgeNodes[1]];
                            onEdge = false;
                            onFace = false;
                        } else {
                            onEdge = true;
                            onFace = false;
                        }
                        curr_pt = pt_i;
                        r_data[nt].push_back( curr_pt );
                    }
                } else { // at Rx, somewhere in a tetrahedron

                    std::array<T2,4> itmp = getPrimary(cellNo);

                    std::array<T2,3> ind[4] = {
                        {itmp[0], itmp[1], itmp[2]},
                        {itmp[0], itmp[1], itmp[3]},
                        {itmp[0], itmp[2], itmp[3]},
                        {itmp[1], itmp[2], itmp[3]}

                    };
                    for ( size_t n=0; n<4; ++n ) {
                        std::sort( ind[n].begin(), ind[n].end() );
                    }

                    bool foundIntersection = false;
                    for ( size_t n=0; n<4; ++n ) {

                        sxyz<T1> pt_i;
                        foundIntersection = intersectVecTriangle(curr_pt, g, ind[n][0],
                                                                 ind[n][1], ind[n][2],
                                                                 pt_i);

                        if ( !foundIntersection )
                            continue;

                        curr_pt = pt_i;

                        bool break_flag = check_pt_location(curr_pt, ind[n], onNode,
                                                            nodeNo, onEdge, edgeNodes,
                                                            onFace, faceNodes);

                        r_data[nt].push_back( curr_pt );

                        if ( break_flag ) break;

                        // find next cell
                        cellNo = findAdjacentCell2(faceNodes, cellNo);
                        if ( cellNo == std::numeric_limits<T2>::max() ) {
                            std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                            << nt << std::endl;
                            r_data[nt].resize(1);
                            r_data[nt][0] = Rx[nt];
                            reachedTx = true;
                        }
                        break;
                    }
                    if ( foundIntersection == false ) {
                        std::cout << "\n\nWarning: finding raypath failed to converge (cell not found) for Tx-Rx pair no "
                        << nt << std::endl;
                        r_data[nt].resize(1);
                        r_data[nt][0] = Rx[nt];
                        reachedTx = true;
                    }
                }

                if ( onNode ) {
                    if ( txOnNode ) {
                        if ( curr_pt.getDistance( Tx[nt] ) < minDist ) {
                            reachedTx = true;
                            break;
                        }
                    } else if ( txOnEdge ) {
                        if ( curr_pt.getDistance(nodes[txEdges[0]]) < minDist ||
                            curr_pt.getDistance(nodes[txEdges[1]]) < minDist ) {

                            r_data[nt].push_back(Tx[nt]);
                            reachedTx = true;
                            break;
                        }
                    } else if ( txOnFace ) {
                        if ( curr_pt.getDistance(nodes[txFaces[0]]) < minDist ||
                            curr_pt.getDistance(nodes[txFaces[1]]) < minDist ||
                            curr_pt.getDistance(nodes[txFaces[2]]) < minDist ) {

                            r_data[nt].push_back(Tx[nt]);
                            reachedTx = true;
                            break;
                        }
                    }
                } if ( onEdge ) {
                    if ( txOnNode ) {
                        if ( txNode == edgeNodes[0] || txNode == edgeNodes[1] ) {
                            r_data[nt].push_back( Tx[nt] );
                            reachedTx = true;
                            break;
                        }
                    } else {
                        std::array<T2,4> itmp = getPrimary(txCell);
                        // find adjacent cells
                        const T2 ind[6][2] = {
                            {itmp[0], itmp[1]},
                            {itmp[0], itmp[2]},
                            {itmp[0], itmp[3]},
                            {itmp[1], itmp[2]},
                            {itmp[1], itmp[3]},
                            {itmp[2], itmp[3]} };
                        for ( size_t ne=0; ne<6; ++ne ) {
                            if ( (ind[ne][0] == edgeNodes[0] && ind[ne][1] == edgeNodes[1]) ||
                                (ind[ne][0] == edgeNodes[1] && ind[ne][1] == edgeNodes[0]) ) {
                                r_data[nt].push_back( Tx[nt] );
                                reachedTx = true;
                                break;
                            }
                        }
                    }
                } else {
                    if ( txOnNode ) {
                        for ( auto nc=nodes[txNode].getOwners().begin(); nc!=nodes[txNode].getOwners().end(); ++nc ) {
                            if ( cellNo == *nc ) {
                                r_data[nt].push_back( Tx[nt] );
                                reachedTx = true;
                                break;
                            }
                        }
                    } else {
                        if ( cellNo == txCell ) {
                            r_data[nt].push_back( Tx[nt] );
                            reachedTx = true;
                        } else {
                            for ( size_t nn=0; nn<txNeighborCells.size(); ++nn ) {
                                if ( cellNo == txNeighborCells[nn] ) {
                                    std::array<T2,4> itmp = getPrimary(txCell);
                                    std::array<T2,3> ind[4] = {
                                        { { itmp[0], itmp[1], itmp[2] } },
                                        { { itmp[0], itmp[1], itmp[3] } },
                                        { { itmp[0], itmp[2], itmp[3] } },
                                        { { itmp[1], itmp[2], itmp[3] } }
                                    };

                                    bool found = false;
                                    for ( size_t n=0; n<4; ++n ) {
                                        std::sort( ind[n].begin(), ind[n].end() );
                                        if ( faceNodes == ind[n] ) {
                                            found = true;
                                            break;
                                        }
                                    }
                                    if ( found ) {
                                        r_data[nt].push_back( Tx[nt] );
                                        reachedTx = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if ( reachedTx ) break;
                }
            }
        }
    }

    template<typename T1, typename T2, typename NODE>
    bool Grid3Duc<T1,T2,NODE>::check_pt_location(sxyz<T1> &curr_pt,
                                                 const std::array<T2,3> &ind,
                                                 bool &onNode,
                                                 T2 &nodeNo,
                                                 bool &onEdge,
                                                 std::array<T2,2> &edgeNodes,
                                                 bool &onFace,
                                                 std::array<T2,3> &faceNodes) const {
        // chech if point is on vertex, edge or face

        bool break_flag = false;

        for ( size_t n=0; n<3; ++n ) {
            if ( nodes[ ind[n] ].getDistance( curr_pt ) < min_dist ) {
                curr_pt = nodes[ ind[n] ];
                nodeNo = ind[n];
                onNode = true;
                onEdge = false;
                onFace = false;
                break_flag = true;
                return break_flag;
            }
        }

        for ( size_t n1=0; n1<3; ++n1 ) {
            size_t n2 = (n1+1)%3;
            if ( areCollinear(curr_pt, nodes[ind[n1]], nodes[ind[n2]]) ) {
                edgeNodes[0] = ind[n1];
                edgeNodes[1] = ind[n2];
                onNode = false;
                onEdge = true;
                onFace = false;
                break_flag = true;
                return break_flag;
            }
        }

        onNode = false;
        onEdge = false;
        onFace = true;

        faceNodes = ind;

        return break_flag;
    }


    template<typename T1, typename T2, typename NODE>
    bool Grid3Duc<T1,T2,NODE>::check_pt_location(sxyz<T1> &curr_pt,
                                                 const std::vector<T2> &ind1,
                                                 const std::array<T2,3> &ind2,
                                                 bool &onNode,
                                                 T2 &nodeNo,
                                                 bool &onEdge,
                                                 std::array<T2,2> &edgeNodes,
                                                 bool &onFace,
                                                 std::array<T2,3> &faceNodes) const {
        // chech if point is on vertex, edge or face

        bool break_flag = false;

        for ( size_t n=0; n<4; ++n ) {
            if ( nodes[ ind1[n] ].getDistance( curr_pt ) < min_dist ) {
                curr_pt = nodes[ ind1[n] ];
                nodeNo = ind1[n];
                onNode = true;
                onEdge = false;
                onFace = false;
                break_flag = true;
                return break_flag;
            }
        }

        for ( size_t n1=0; n1<3; ++n1 ) {
            size_t n2 = (n1+1)%3;
            if ( areCollinear(curr_pt, nodes[ind2[n1]], nodes[ind2[n2]]) ) {
                edgeNodes[0] = ind2[n1];
                edgeNodes[1] = ind2[n2];
                onNode = false;
                onEdge = true;
                onFace = false;
                break_flag = true;
                return break_flag;
            }
        }

        onNode = false;
        onEdge = false;
        onFace = true;

        faceNodes = ind2;
        std::sort(faceNodes.begin(), faceNodes.end());

        return break_flag;
    }


    template<typename T1, typename T2, typename NODE>
    bool Grid3Duc<T1,T2,NODE>::intersectVecTriangle(const T2 iO, const sxyz<T1> &vec,
                                                    const T2 iA, T2 iB, T2 iC,
                                                    sxyz<T1> &pt_i) const {

        sxyz<T1> OA = {nodes[iA].getX()-nodes[iO].getX(), nodes[iA].getY()-nodes[iO].getY(), nodes[iA].getZ()-nodes[iO].getZ()};
        // check if counterclockwise
        sxyz<T1> AB = {nodes[iB].getX()-nodes[iA].getX(),
            nodes[iB].getY()-nodes[iA].getY(),
            nodes[iB].getZ()-nodes[iA].getZ()};
        sxyz<T1> AC = {nodes[iC].getX()-nodes[iA].getX(),
            nodes[iC].getY()-nodes[iA].getY(),
            nodes[iC].getZ()-nodes[iA].getZ()};
        sxyz<T1> n = cross(AB, AC);
        if ( dot(OA, n) > 0. ) std::swap(iB, iC);

        sxyz<T1> OB = {nodes[iB].getX()-nodes[iO].getX(), nodes[iB].getY()-nodes[iO].getY(), nodes[iB].getZ()-nodes[iO].getZ()};
        sxyz<T1> OC = {nodes[iC].getX()-nodes[iO].getX(), nodes[iC].getY()-nodes[iO].getY(), nodes[iC].getZ()-nodes[iO].getZ()};

        T1 u, v, w;
        u = tripleScalar(vec, OC, OB);
        if ( u<0.0 ) return false;
        v = tripleScalar(vec, OA, OC);
        if ( v<0.0 ) return false;
        w = tripleScalar(vec, OB, OA);
        if ( w<0.0 ) return false;

        T1 den = 1./(u+v+w);
        u *= den;
        v *= den;
        w *= den;

        pt_i.x = u*nodes[iA].getX() + v*nodes[iB].getX() + w*nodes[iC].getX();
        pt_i.y = u*nodes[iA].getY() + v*nodes[iB].getY() + w*nodes[iC].getY();
        pt_i.z = u*nodes[iA].getZ() + v*nodes[iB].getZ() + w*nodes[iC].getZ();

        return true;
    }

    template<typename T1, typename T2, typename NODE>
    bool Grid3Duc<T1,T2,NODE>::intersectVecTriangle(const sxyz<T1> &O, const sxyz<T1> &vec,
                                                    const T2 iA, T2 iB, T2 iC,
                                                    sxyz<T1> &pt_i) const {

        sxyz<T1> OA = {nodes[iA].getX()-O.x, nodes[iA].getY()-O.y, nodes[iA].getZ()-O.z};
        // check if counterclockwise
        sxyz<T1> AB = {nodes[iB].getX()-nodes[iA].getX(),
            nodes[iB].getY()-nodes[iA].getY(),
            nodes[iB].getZ()-nodes[iA].getZ()};
        sxyz<T1> AC = {nodes[iC].getX()-nodes[iA].getX(),
            nodes[iC].getY()-nodes[iA].getY(),
            nodes[iC].getZ()-nodes[iA].getZ()};
        sxyz<T1> n = cross(AB, AC);
        if ( dot(OA, n) > 0. ) std::swap(iB, iC);

        sxyz<T1> OB = {nodes[iB].getX()-O.x, nodes[iB].getY()-O.y, nodes[iB].getZ()-O.z};
        sxyz<T1> OC = {nodes[iC].getX()-O.x, nodes[iC].getY()-O.y, nodes[iC].getZ()-O.z};

        T1 u, v, w;
        u = tripleScalar(vec, OC, OB);
        if ( u<0.0 ) return false;
        v = tripleScalar(vec, OA, OC);
        if ( v<0.0 ) return false;
        w = tripleScalar(vec, OB, OA);
        if ( w<0.0 ) return false;

        T1 den = 1./(u+v+w);
        u *= den;
        v *= den;
        w *= den;

        pt_i.x = u*nodes[iA].getX() + v*nodes[iB].getX() + w*nodes[iC].getX();
        pt_i.y = u*nodes[iA].getY() + v*nodes[iB].getY() + w*nodes[iC].getY();
        pt_i.z = u*nodes[iA].getZ() + v*nodes[iB].getZ() + w*nodes[iC].getZ();

        return true;
    }

    template<typename T1, typename T2, typename NODE>
    bool Grid3Duc<T1,T2,NODE>::intersectVecEdge(const sxyz<T1>& curr_pt,
                                                const sxyz<T1>& g,
                                                std::array<T2,3>& faceNodes,
                                                sxyz<T1>&  pt_i,
                                                std::array<T2,2>& edgeNodes) const {

        // from http://mathworld.wolfram.com/Line-LineIntersection.html

#ifdef DEBUG_RP
        std::cout << "\n\n\n\n\nIn intersectVecEdge (face)\n\n";

        std::cout << "fig = plt.figure()\nax = fig.add_subplot(111, projection='3d')\n";
        std::cout << "cpt = np.array([" << curr_pt.x << ", " << curr_pt.y << ", " << curr_pt.z << "])\n";
        std::cout << "g = np.array([" << g.x << ", " << g.y << ", " << g.z << "])\n";
        std::cout << "pt2 = cpt + 0.05*g\n";
        std::cout << "c1 = np.array([" << nodes[faceNodes[0]].getX() << ", " << nodes[faceNodes[0]].getY() << ", " << nodes[faceNodes[0]].getZ() << "])\n";
        std::cout << "c2 = np.array([" << nodes[faceNodes[1]].getX() << ", " << nodes[faceNodes[1]].getY() << ", " << nodes[faceNodes[1]].getZ() << "])\n";
        std::cout << "c3 = np.array([" << nodes[faceNodes[2]].getX() << ", " << nodes[faceNodes[2]].getY() << ", " << nodes[faceNodes[2]].getZ() << "])\n";

        std::cout << "ax.plot([c1[0], c2[0], c3[0], c1[0]], [c1[1], c2[1], c3[1], c1[1]], [c1[2], c2[2], c3[2], c1[2]])\n";
        std::cout << "ax.plot([cpt[0], pt2[0]], [cpt[1], pt2[1]], [cpt[2], pt2[2]], c='r')\n";
        std::cout << "ax.scatter(cpt[0], cpt[1], cpt[2], c='r')\n";
#endif

        sxyz<T1> x1 = {nodes[faceNodes[0]].getX(),
            nodes[faceNodes[0]].getY(),
            nodes[faceNodes[0]].getZ()};
        sxyz<T1> x2 = {nodes[faceNodes[1]].getX(),
            nodes[faceNodes[1]].getY(),
            nodes[faceNodes[1]].getZ()};
        sxyz<T1> x4 = curr_pt + static_cast<T1>(10.0)*x1.getDistance(x2) * g;

        sxyz<T1> a = x2 - x1;
        sxyz<T1> b = x4 - curr_pt;
        sxyz<T1> c = curr_pt - x1;

        sxyz<T1> ab = cross(a, b);

        pt_i = x1 + (dot(cross(c, b), ab) / norm2(ab)) * a;

#ifdef DEBUG_RP
        std::cout << "pti = np.array([" << pt_i.x << ", " << pt_i.y << ", " << pt_i.z << "])\n";
        std::cout << "ax.scatter(pti[0], pti[1], pti[2], c='g')\n";
#endif

        sxyz<T1> mid_pt = static_cast<T1>(0.5) * ( curr_pt + pt_i );

        bool test1 = testInTriangle(&(nodes[ faceNodes[0] ]),
                                    &(nodes[ faceNodes[1] ]),
                                    &(nodes[ faceNodes[2] ]), mid_pt);

        // check if we are between x1 & x2

        b = pt_i - x1;
        T1 dab = dot(a, b);
        bool test2 = dab > 0.0 && dab <= norm2(a);

        // check if going in the same direction as g

        b = pt_i - curr_pt;
        bool test3 = dot(b, g) > 0.0;

        if ( test1 && test2 && test3) {
            edgeNodes[0] = faceNodes[0];
            edgeNodes[1] = faceNodes[1];
            return true;
        }

        x1 = {nodes[faceNodes[0]].getX(),
            nodes[faceNodes[0]].getY(),
            nodes[faceNodes[0]].getZ()};
        x2 = {nodes[faceNodes[2]].getX(),
            nodes[faceNodes[2]].getY(),
            nodes[faceNodes[2]].getZ()};

        a = x2 - x1;
        b = x4 - curr_pt;
        c = curr_pt - x1;

        ab = cross(a, b);

        pt_i = x1 + (dot(cross(c, b), ab) / norm2(ab)) * a;

#ifdef DEBUG_RP
        std::cout << "pti = np.array([" << pt_i.x << ", " << pt_i.y << ", " << pt_i.z << "])\n";
        std::cout << "ax.scatter(pti[0], pti[1], pti[2], c='k')\n";
#endif

        mid_pt = static_cast<T1>(0.5) * ( curr_pt + pt_i );

        test1 = testInTriangle(&(nodes[ faceNodes[0] ]),
                               &(nodes[ faceNodes[1] ]),
                               &(nodes[ faceNodes[2] ]), mid_pt);
        // check if we are between x1 & x2

        b = pt_i - x1;
        dab = dot(a, b);
        test2 = dab > 0.0 && dab <= norm2(a);

        b = pt_i - curr_pt;
        test3 = dot(b, g) > 0.0;

        if ( test1 && test2 && test3 ) {
            edgeNodes[0] = faceNodes[0];
            edgeNodes[1] = faceNodes[2];
            return true;
        }

        x1 = {nodes[faceNodes[1]].getX(),
            nodes[faceNodes[1]].getY(),
            nodes[faceNodes[1]].getZ()};
        x2 = {nodes[faceNodes[2]].getX(),
            nodes[faceNodes[2]].getY(),
            nodes[faceNodes[2]].getZ()};

        a = x2 - x1;
        b = x4 - curr_pt;
        c = curr_pt - x1;

        ab = cross(a, b);

        pt_i = x1 + (dot(cross(c, b), ab) / norm2(ab)) * a;

#ifdef DEBUG_RP
        std::cout << "pti = np.array([" << pt_i.x << ", " << pt_i.y << ", " << pt_i.z << "])\n";
        std::cout << "ax.scatter(pti[0], pti[1], pti[2], c='b')\n";
        std::cout << "plt.show()\n\n";
#endif

        mid_pt = static_cast<T1>(0.5) * ( curr_pt + pt_i );

        test1 = testInTriangle(&(nodes[ faceNodes[0] ]),
                               &(nodes[ faceNodes[1] ]),
                               &(nodes[ faceNodes[2] ]), mid_pt);
        b = pt_i - x1;
        dab = dot(a, b);
        test2 = dab > 0.0 && dab <= norm2(a);

        b = pt_i - curr_pt;
        test3 = dot(b, g) > 0.0;

        if ( test1 && test2 && test3 ) {
            edgeNodes[0] = faceNodes[1];
            edgeNodes[1] = faceNodes[2];
            return true;
        }

        return false;
    }

    template<typename T1, typename T2, typename NODE>
    T2 Grid3Duc<T1,T2,NODE>::findAdjacentCell1(const std::array<T2,3> &faceNodes,
                                               const T2 nodeNo) const {

        std::vector<T2> cells;
        for ( auto nc0=nodes[faceNodes[0]].getOwners().begin(); nc0!=nodes[faceNodes[0]].getOwners().end(); ++nc0 ) {
            if ( std::find(nodes[faceNodes[1]].getOwners().begin(), nodes[faceNodes[1]].getOwners().end(), *nc0)!=nodes[faceNodes[1]].getOwners().end() &&
                std::find(nodes[faceNodes[2]].getOwners().begin(), nodes[faceNodes[2]].getOwners().end(), *nc0)!=nodes[faceNodes[2]].getOwners().end() )
                cells.push_back( *nc0 );
        }
        if ( cells.size() == 1 ) {
            return cells[0];
        }
        for ( auto nc0=nodes[nodeNo].getOwners().begin(); nc0!=nodes[nodeNo].getOwners().end(); ++nc0 ) {
            if ( *nc0 == cells[0] ) {
                return cells[1];
            } else if ( *nc0 == cells[1] ) {
                return cells[0];
            }
        }
        return std::numeric_limits<T2>::max();
    }

    template<typename T1, typename T2, typename NODE>
    T2 Grid3Duc<T1,T2,NODE>::findAdjacentCell2(const std::array<T2,3> &faceNodes,
                                               const T2 cellNo) const {

        std::vector<T2> cells;
        for ( auto nc0=nodes[faceNodes[0]].getOwners().begin(); nc0!=nodes[faceNodes[0]].getOwners().end(); ++nc0 ) {
            if ( std::find(nodes[faceNodes[1]].getOwners().begin(), nodes[faceNodes[1]].getOwners().end(), *nc0)!=nodes[faceNodes[1]].getOwners().end() &&
                std::find(nodes[faceNodes[2]].getOwners().begin(), nodes[faceNodes[2]].getOwners().end(), *nc0)!=nodes[faceNodes[2]].getOwners().end() )
                cells.push_back( *nc0 );
        }
        if ( cells.size() == 1 ) {
            return cells[0];
        }
        if ( cellNo == cells[0] ) {
            return cells[1];
        } else if ( cellNo == cells[1] ) {
            return cells[0];
        }
        return std::numeric_limits<T2>::max();
    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::plotCell(const T2 cellNo, const sxyz<T1> &pt, const sxyz<T1> &g) const {


        if ( true ) {
            std::array<T2,4> itmp = getPrimary(cellNo);
            T2 i0 = itmp[0];
            T2 i1 = itmp[1];
            T2 i2 = itmp[2];
            T2 i3 = itmp[3];

            std::cout << "\nplot3(["<<nodes[ i0 ].getX()<<' ' << nodes[ i1 ].getX() <<"],["
            <<nodes[ i0 ].getY()<<' ' << nodes[ i1 ].getY() <<"],["
            <<nodes[ i0 ].getZ()<<' ' << nodes[ i1 ].getZ() <<"]); hold on;\n";
            std::cout << "plot3(["<<nodes[ i0 ].getX()<<' ' << nodes[ i2 ].getX() <<"],["
            <<nodes[ i0 ].getY()<<' ' << nodes[ i2 ].getY() <<"],["
            <<nodes[ i0 ].getZ()<<' ' << nodes[ i2 ].getZ() <<"])\n";
            std::cout << "plot3(["<<nodes[ i0 ].getX()<<' ' << nodes[ i3 ].getX() <<"],["
            <<nodes[ i0 ].getY()<<' ' << nodes[ i3 ].getY() <<"],["
            <<nodes[ i0 ].getZ()<<' ' << nodes[ i3 ].getZ() <<"])\n";
            std::cout << "plot3(["<<nodes[ i1 ].getX()<<' '<<nodes[ i2 ].getX()<<' '<<nodes[ i3 ].getX()<<' '<<nodes[ i1 ].getX()<<"],["
            <<nodes[ i1 ].getY()<<' '<<nodes[ i2 ].getY()<<' '<<nodes[ i3 ].getY()<<' '<<nodes[ i1 ].getY()<<"],["
            <<nodes[ i1 ].getZ()<<' '<<nodes[ i2 ].getZ()<<' '<<nodes[ i3 ].getZ()<<' '<<nodes[ i1 ].getZ()<<"])\n";
            std::cout << "plot3(["<<pt.x<< ' ' << pt.x+g.x<<"],["<<pt.y<< ' ' << pt.y+g.y<<"],["<<pt.z<< ' ' << pt.z+g.z<<"],'r')\naxis equal\n\n";
        }
    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::getNeighborNodes(const T2 cellNo,
                                                std::set<NODE*> &nnodes) const {

        for ( size_t n=0; n<this->neighbors[cellNo].size(); ++n ) {
            if ( nodes[this->neighbors[cellNo][n]].isPrimary() ) {
                T2 nodeNo = this->neighbors[cellNo][n];
                nnodes.insert( &(nodes[nodeNo]) );
                if ( rp_method == 1 ) {
                    for ( auto nc=nodes[nodeNo].getOwners().cbegin(); nc!=nodes[nodeNo].getOwners().cend(); ++nc ) {
                        for ( size_t nn=0; nn<this->neighbors[*nc].size(); ++nn ) {
                            if ( nodes[ this->neighbors[*nc][nn] ].isPrimary() ) {
                                nnodes.insert( &(nodes[ this->neighbors[*nc][nn] ]) );
                            }
                        }
                    }
                }
            }
        }
    }

    template<typename T1, typename T2, typename NODE>
    void Grid3Duc<T1,T2,NODE>::getNeighborNodesAB(const std::vector<NODE*>& ref_pt,
                                                  std::vector<std::vector<std::array<NODE*,3>>>& opp_pts) const {
        opp_pts.resize( ref_pt.size() );
        for ( size_t nr=0; nr<ref_pt.size(); ++nr ) {
            opp_pts[nr].resize( ref_pt[nr]->getOwners().size() );
            for ( size_t nc=0; nc<ref_pt[nr]->getOwners().size(); ++nc ) {
                T2 cellNo =ref_pt[nr]->getOwners()[nc];
                size_t ind=0;
                for ( size_t nn=0; nn<4; ++nn ) {
                    if ( &(nodes[ this->neighbors[cellNo][nn] ]) != ref_pt[nr] && nodes[ this->neighbors[cellNo][nn] ].isPrimary() ) {
                        opp_pts[nr][nc][ind++] = &(nodes[ this->neighbors[cellNo][nn] ]);
                    }
                }
            }
        }
    }

    template<typename T1, typename T2, typename NODE>
    sxyz<T1> Grid3Duc<T1,T2,NODE>::projectOnFace(const sxyz<T1>& curr_pt,
                                                 const sxyz<T1>& g,
                                                 std::array<T2,2>& edgeNodes,
                                                 const std::vector<T2> cells,
                                                 sxyz<T1>&  pt_i) const {
        // find all primary nodes of common cells
        std::vector<std::array<T2,4>> primary(cells.size());
        for (size_t nc=0; nc<cells.size(); ++nc) {
            size_t np = 0;
            for ( auto nn=this->neighbors[cells[nc]].begin(); nn!= this->neighbors[cells[nc]].end(); ++nn ) {
                if (nodes[*nn].isPrimary()) {
                    primary[nc][np++] = *nn;
                }
            }
        }
        // at the surface, the edge is common to two triangles
        // we must have only two other nodes at the surface, which are not common to the cells
        std::array<T2,2> edgeNodes2;
        size_t ne = 0;
        for (size_t nc=0; nc<cells.size(); ++nc) {
            for (size_t np=0; np<4; ++np) {
                if (primary[nc][np] == edgeNodes[0] || primary[nc][np] == edgeNodes[1]) {
                    continue;
                }
                bool found=false;
                for (size_t nc2=0; nc2<cells.size(); ++nc2) {
                    if ( nc == nc2 ) {
                        continue;
                    }
                    if ( std::find(primary[nc2].begin(), primary[nc2].end(), primary[nc][np])!=primary[nc2].end() ) {
                        found = true;  // primary node is common to two cells
                        break;
                    }
                }
                if ( found == false ) {
                    edgeNodes2[ne++] = primary[nc][np];
                }
            }
        }

        sxyz<T1> g_proj;
        T1 minAngle = std::numeric_limits<T1>::max();
        std::array<T2,2> oldEdgeNodes = edgeNodes;
        // find projection that is closest to current gradient
        for (auto en=edgeNodes2.begin(); en!=edgeNodes2.end(); ++en) {

            std::array<T2,2> tmpEdgeNodes = oldEdgeNodes;

            sxyz<T1> tmp = projectOnFace(g, {tmpEdgeNodes[0], tmpEdgeNodes[1], *en});
            T1 a2 = acos(dot(tmp, g)/(norm(tmp)*norm(g)));

#ifdef DEBUG_RP
            if ( verbose > 1 ) {
                std::cout << "\n\n\n\n\n# In projectOnFace\n\n";
                std::cout << "fig = plt.figure()\nax = fig.add_subplot(111, projection='3d')\n";
                std::cout << "cpt = np.array([" << curr_pt.x << ", " << curr_pt.y << ", " << curr_pt.z << "])\n";
                std::cout << "g = np.array([" << g.x << ", " << g.y << ", " << g.z << "])\n";
                std::cout << "pt2 = cpt + 0.05*g\n";
                std::cout << "c1 = np.array([" << nodes[tmpEdgeNodes[0]].getX() << ", " << nodes[tmpEdgeNodes[0]].getY() << ", " << nodes[tmpEdgeNodes[0]].getZ() << "])\n";
                std::cout << "c2 = np.array([" << nodes[tmpEdgeNodes[1]].getX() << ", " << nodes[tmpEdgeNodes[1]].getY() << ", " << nodes[tmpEdgeNodes[1]].getZ() << "])\n";
                std::cout << "g2 = np.array([" << tmp.x << ", " << tmp.y << ", " << tmp.z << "])\n";
                std::cout << "c3 = np.array([" << nodes[*en].getX() << ", " << nodes[*en].getY() << ", " << nodes[*en].getZ() << "])\n";
                std::cout << "a2 = " << a2 << "\n";
                std::cout << "pt3 = cpt + 0.05*g2\n";
                std::cout << "ax.plot([c1[0], c2[0], c3[0], c1[0]], [c1[1], c2[1], c3[1], c1[1]], [c1[2], c2[2], c3[2], c1[2]])\n";
                std::cout << "ax.plot([cpt[0], pt2[0]], [cpt[1], pt2[1]], [cpt[2], pt2[2]], c='g')\n";
                std::cout << "ax.plot([cpt[0], pt3[0]], [cpt[1], pt3[1]], [cpt[2], pt3[2]], c='k')\n";
                std::cout << "ax.scatter(cpt[0], cpt[1], cpt[2], c='r')\n";
                std::cout << "ax.scatter(c1[0], c1[1], c1[2], c='k')\n";
                std::cout << "ax.scatter(c2[0], c2[1], c2[2], c='k')\n";
                std::cout << "ax.scatter(pt2[0], pt2[1], pt2[2], c='g')\n";
            }
#endif
            if ( a2 < minAngle ) {
                // compute intersection point and check if within edge nodes

                sxyz<T1> x1 = {nodes[tmpEdgeNodes[0]].getX(),
                    nodes[tmpEdgeNodes[0]].getY(),
                    nodes[tmpEdgeNodes[0]].getZ()};
                sxyz<T1> x2 = {nodes[*en].getX(),
                    nodes[*en].getY(),
                    nodes[*en].getZ()};
                sxyz<T1> x4 = curr_pt + static_cast<T1>(10.0)*x1.getDistance(x2) * tmp;

                sxyz<T1> a = x2 - x1;
                sxyz<T1> b = x4 - curr_pt;
                sxyz<T1> c = curr_pt - x1;

                sxyz<T1> ab = cross(a, b);

                sxyz<T1> pt_i0 = x1 + (dot(cross(c, b), ab) / norm2(ab)) * a;

                sxyz<T1> mid_pt = static_cast<T1>(0.5) * ( curr_pt + pt_i0 );

                bool test1 = testInTriangle(&(nodes[ tmpEdgeNodes[0] ]),
                                            &(nodes[ tmpEdgeNodes[1] ]),
                                            &(nodes[ *en ]), mid_pt);

                // check if we are between x1 & x2

                b = pt_i0 - x1;
                T1 dab = dot(a, b);
                bool test2 = dab > 0.0 && norm2(b) <= norm2(a);

                // check if going in the same direction as g

                b = pt_i0 - curr_pt;
                T1 theta = acos(dot(b, g) / (norm(b)*norm(g)));
#ifdef DEBUG_RP
                if ( verbose > 1 ) {
                    std::cout << "pti = np.array([" << pt_i0.x << ", " << pt_i0.y << ", " << pt_i0.z << "])\n";
                    std::cout << "ax.scatter(pti[0], pti[1], pti[2], c='g')\n";
                    std::cout << "th = " << theta*180.0/3.14159 << '\n';
                }
#endif
                bool test3 = theta < theta_cut;  // defined in ttcr_t.h

                if ( test1 && test2 && test3 ) {
                    pt_i = pt_i0;
                    edgeNodes[0] = tmpEdgeNodes[0];
                    edgeNodes[1] = *en;
                    g_proj = tmp;
                    minAngle = a2;
#ifdef DEBUG_RP
                    if ( verbose > 1 ) {
                        std::cout << "\n\n\n\n\n# found\n\nfig = plt.figure()\nax = fig.add_subplot(111, projection='3d')\n";
                        std::cout << "pti = np.array([" << pt_i0.x << ", " << pt_i0.y << ", " << pt_i0.z << "])\n";
                        std::cout << "c1 = np.array([" << nodes[edgeNodes[0]].getX() << ", " << nodes[edgeNodes[0]].getY() << ", " << nodes[edgeNodes[0]].getZ() << "])\n";
                        std::cout << "c2 = np.array([" << nodes[edgeNodes[1]].getX() << ", " << nodes[edgeNodes[1]].getY() << ", " << nodes[edgeNodes[1]].getZ() << "])\n";
                        std::cout << "ax.plot([c1[0], c2[0]], [c1[1], c2[1]], [c1[2], c2[2]], 'k')\n";

                        std::cout << "ax.scatter(pti[0], pti[1], pti[2], c='r')\n";
                        std::cout << "ax.scatter(c1[0], c1[1], c1[2], c='k')\n";
                        std::cout << "ax.scatter(c2[0], c2[1], c2[2], c='k')\n";
                        std::cout << "plt.show()\n\n\n\n\n\n";
                    }
#endif
                    continue;
                }

                x1 = {nodes[tmpEdgeNodes[1]].getX(),
                    nodes[tmpEdgeNodes[1]].getY(),
                    nodes[tmpEdgeNodes[1]].getZ()};
                x2 = {nodes[*en].getX(),
                    nodes[*en].getY(),
                    nodes[*en].getZ()};

                a = x2 - x1;
                b = x4 - curr_pt;
                c = curr_pt - x1;

                ab = cross(a, b);

                pt_i0 = x1 + (dot(cross(c, b), ab) / norm2(ab)) * a;

                mid_pt = static_cast<T1>(0.5) * ( curr_pt + pt_i0 );

                test1 = testInTriangle(&(nodes[ tmpEdgeNodes[0] ]),
                                       &(nodes[ tmpEdgeNodes[1] ]),
                                       &(nodes[ *en ]), mid_pt);

                b = pt_i0 - x1;
                dab = dot(a, b);
                test2 = dab > 0.0 && dab <= norm2(a);

                b = pt_i0 - curr_pt;
                theta = acos(dot(b, g) / (norm(b)*norm(g)));
#ifdef DEBUG_RP
                if ( verbose > 1 ) {
                    std::cout << "pti = np.array([" << pt_i0.x << ", " << pt_i0.y << ", " << pt_i0.z << "])\n";
                    std::cout << "ax.scatter(pti[0], pti[1], pti[2], c='k')\n";
                    std::cout << "plt.show()\n\n";
                    std::cout << "th = " << theta*180.0/3.14159 << '\n';
                }
#endif
                test3 = theta < theta_cut;  // defined in ttcr_t.h

                if ( test1 && test2 && test3 ) {
                    pt_i = pt_i0;
                    edgeNodes[0] = *en;
                    edgeNodes[1] = tmpEdgeNodes[1];
                    g_proj = tmp;
                    minAngle = a2;
#ifdef DEBUG_RP
                    if ( verbose > 1 ) {
                        std::cout << "\n\n\n\n\n# found\n\nfig = plt.figure()\nax = fig.add_subplot(111, projection='3d')\n";
                        std::cout << "pti = np.array([" << pt_i0.x << ", " << pt_i0.y << ", " << pt_i0.z << "])\n";
                        std::cout << "c1 = np.array([" << nodes[edgeNodes[0]].getX() << ", " << nodes[edgeNodes[0]].getY() << ", " << nodes[edgeNodes[0]].getZ() << "])\n";
                        std::cout << "c2 = np.array([" << nodes[edgeNodes[1]].getX() << ", " << nodes[edgeNodes[1]].getY() << ", " << nodes[edgeNodes[1]].getZ() << "])\n";
                        std::cout << "ax.plot([c1[0], c2[0]], [c1[1], c2[1]], [c1[2], c2[2]], 'k')\n";
                        std::cout << "ax.scatter(pti[0], pti[1], pti[2], c='r')\n";
                        std::cout << "ax.scatter(c1[0], c1[1], c1[2], c='k')\n";
                        std::cout << "ax.scatter(c2[0], c2[1], c2[2], c='k')\n";
                        std::cout << "plt.show()\n\n\n\n\n\n";
                    }
#endif
                    continue;
                }
            }
        }
        return g_proj;
    }

    template<typename T1, typename T2, typename NODE>
    bool Grid3Duc<T1,T2,NODE>::projectOnFace(const sxyz<T1>& curr_pt,
                                             const T2 nodeNo,
                                             sxyz<T1>& g,
                                             std::array<T2,2>& edgeNodes,
                                             sxyz<T1>& pt_i) const {

        std::set<std::array<T2,3>> faces;  // for some reason, unordered_set does not compile

        // loop over cells to find faces connected to current point
        for ( auto nc=nodes[nodeNo].getOwners().begin(); nc!=nodes[nodeNo].getOwners().end(); ++nc ) {
            // find nodes other that nodeNo
            std::array<T2,3> tmpnodes;
            size_t n = 0;
            for ( size_t nn=0; nn<4; ++nn ) {
                if ( nodeNo != tetrahedra[*nc].i[nn] ) {
                    tmpnodes[n++] = tetrahedra[*nc].i[nn];
                }
            }

            std::sort(tmpnodes.begin(), tmpnodes.end());
            faces.insert({nodeNo, tmpnodes[0], tmpnodes[1]});
            faces.insert({nodeNo, tmpnodes[0], tmpnodes[2]});
            faces.insert({nodeNo, tmpnodes[1], tmpnodes[2]});
        }

        bool found = false;
        sxyz<T1> g_old = g;
        T1 minAngle = 9999.9;
        T1 ng = norm(g);
        // find projection that is closest to current gradient
        for ( auto fn=faces.begin(); fn!=faces.end(); ++fn ) {
            sxyz<T1> gtmp = projectOnFace(g_old, *fn);

            // find pt of intersection with opposing edge

            sxyz<T1> x1 = {nodes[(*fn)[1]].getX(),
                nodes[(*fn)[1]].getY(),
                nodes[(*fn)[1]].getZ()};
            sxyz<T1> x2 = {nodes[(*fn)[2]].getX(),
                nodes[(*fn)[2]].getY(),
                nodes[(*fn)[2]].getZ()};
            sxyz<T1> x4 = curr_pt + static_cast<T1>(10.0)*x1.getDistance(x2) * gtmp;

            sxyz<T1> a = x2 - x1;
            sxyz<T1> b = x4 - curr_pt;
            sxyz<T1> c = curr_pt - x1;

            sxyz<T1> ab = cross(a, b);

            sxyz<T1> pt_i2 = x1 + (dot(cross(c, b), ab) / norm2(ab)) * a;

            // check if pt_i is between x1 and x2

            b = pt_i2 - x1;
            T1 dab = dot(a, b);

            // check if going in the same direction as g
            c = pt_i2 - curr_pt;

            T1 dcg = dot(c, g_old);
            T1 angle = std::acos(dcg/(norm(c)*ng));

            if ( dab > 0.0 && norm2(b) <= norm2(a) && dcg > 0.0 && angle < minAngle ) {
                g = gtmp;
                pt_i = pt_i2;
                edgeNodes[0] = (*fn)[1];
                edgeNodes[1] = (*fn)[2];

                minAngle = angle;
                found = true;
            }
        }
        return found;
    }

    template<typename T1, typename T2, typename NODE>
    const T1 Grid3Duc<T1,T2,NODE>::getAverageEdgeLength() const {
        std::set<std::array<T2,2>> edges;
        typename std::set<std::array<T2,2>>::iterator edgIt;
        T2 iNodes[6][2] = {
            {0,1},
            {0,2},
            {0,3},
            {1,2},
            {1,3},
            {2,3}
        };
        T1 sum = 0.0;
        for (size_t ntet=0; ntet<tetrahedra.size(); ++ntet) {
            for (size_t n=0; n<6; ++n) {
                std::array<T2, 2> edgei = {tetrahedra[ntet].i[iNodes[n][0]],
                    tetrahedra[ntet].i[iNodes[n][1]]};
                std::sort(edgei.begin(), edgei.end());
                edgIt = edges.find(edgei);
                if ( edgIt  == edges.end() ) {
                    T1 d = nodes[edgei[0]].getDistance(nodes[edgei[1]]);
                    sum += d;
                    edges.insert(edgei);
                }
            }
        }
        return (sum/edges.size());
    }

}

#endif
