#include "fg/gc/generalisedcylinder.h"

#include <iostream>

using namespace std;

namespace fg {
    namespace gc {
        GeneralisedCylinder::GeneralisedCylinder( const CarrierCurve &carrier, const vector<Mat4> &orients, const CrossSection &crossSection,
												  const std::vector< std::pair<double,double> > &csDomains,
												  const Interpolator<double> &scale,
												  const std::vector< std::pair<double,double> > &scaleDomains, const std::vector< int > &strips )
            : mCarrier( carrier )
            , mCrossSection( crossSection )
			, mScale(scale)
			, mStrips(strips)
        {
            std::vector<Quat> newOrients;

            for( int i = 0; i < orients.size(); ++i )
            {
                newOrients.push_back( Quat( orients[i] ) );
            }

            updateOrients( newOrients );
			mCSDomain = csDomains;
			mScaleDomain = scaleDomains;
        }

        GeneralisedCylinder::GeneralisedCylinder( const CarrierCurve &carrier, const vector<Mat4> &orients, const CrossSection &crossSection,
												  const std::vector< std::pair<double,double> > &csDomains,
												  const Interpolator<double> &scale,
												  const std::vector< std::pair<double,double> > &scaleDomains )
            : mCarrier( carrier )
            , mCrossSection( crossSection )
			, mScale(scale)
        {
            std::vector<Quat> newOrients;

            for( int i = 0; i < orients.size(); ++i )
            {
                newOrients.push_back( Quat( orients[i] ) );
            }

            updateOrients( newOrients );
			mCSDomain = csDomains;
			mScaleDomain = scaleDomains;

			for( int i = 0; i < carrier.getInterpolator()->getNumControlPoints() - 1; ++i ) {
				mStrips.push_back( 5 );
			}
        }

        void GeneralisedCylinder::updateOrients( const vector<Quat> &orients )
        {
            mOrients.clear();

            // Store the difference between the specified orientations and the one supplied by the carrier curve
            for( int i = 0; i < mCarrier.getInterpolator()->getNumSegments(); ++i )
            {
                pair<Quat, Quat> co = mCarrier.getSegOrients( i );
                pair<Quat, Quat> no;
                no.first = orients[i] * co.first.inverse();
                no.second = orients[i + 1] * co.second.inverse();
                mOrients.push_back( no );
            }
        }

        Vec3 GeneralisedCylinder::getPosition( double u, double v ) const
        {
			double csv = getCrossSectionV( v );
            // Position on cross section
            Vec3 cs = mCrossSection.getPosition( u, csv ) * mScale.getPosition( getScaleV( v ) );
            // Carriers frame rotation
            Quat cr = mCarrier.orient( v );
            // Additional rotation
            int vint = ( int ) v;
            vint = clamp<int>( vint, 0, mCarrier.getInterpolator()->getNumSegments() - 1 );
            double vfrac = v - vint;
            Quat ar = mOrients[vint].first.slerp( vfrac, mOrients[vint].second );
            // Put it all together
            return mCarrier.getInterpolator()->getPosition( v ) + ar * cr * cs;
        }

        Quat GeneralisedCylinder::orient( double v ) const
        {
            // Carriers frame rotation
            Quat cr = mCarrier.orient( v );
            // Additional rotation
            int vint = ( int ) v;
            vint = clamp<int>( vint, 0, mCarrier.getInterpolator()->getNumSegments() - 1 );
            double vfrac = v - vint;
            Quat ar = mOrients[vint].first.slerp( vfrac, mOrients[vint].second );
            return Quat( ar * cr );
        }

        void GeneralisedCylinder::createMesh( Mesh::MeshBuilder &mb ) const
        {
			int m = mStrips[0];
			int oldVerticies = mb.getNumVerticies();
            double vmin, vmax;
			double csv, sv, scale;
            mCarrier.getInterpolator()->getDomain( vmin, vmax );
            double vinc;
            double v = 0.;
            int pCsIndex, nCsIndex;
			double pTLength, nTLength, pRTLength, nRTLength, pLength, nLength;

            // Get the first cross section add add the verticies to the mesh
			csv = getCrossSectionV( v );
            sv = getScaleV( v );
			scale = mScale.getPosition( sv );
            vector<Vec3> pCs = mCrossSection.getCrossSection( csv, scale );
			nTLength = getTotalLengthSquared( pCs );
			nLength = 0.;
            vector<Vec3> nCs;
            Quat ori;
            ori = orient( v );
            Vec3 cVert;
            Vec3 cCpos = mCarrier.getInterpolator()->getPosition( v );

            for( int i = 0; i < pCs.size(); ++i )
            {
                cVert = cCpos + ori * pCs[i];
                mb.addVertex( cVert.getX(), cVert.getY(), cVert.getZ() );
            }

            pCsIndex = oldVerticies;
            nCsIndex = pCs.size() + oldVerticies;

            // Add on control point at a time
			for( int l = 0; l < mCarrier.getInterpolator()->getNumControlPoints() - 1; ++l )
			{
				m = mStrips[l];
				vinc = 1. / ( ( double ) mStrips[l] );

            	for( int k = 0; k < m; ++k ) {
            	    // Get the next cross section and add it's verticies to the mesh
					csv = getCrossSectionV( v + vinc );
            		sv = getScaleV( v );
					scale = mScale.getPosition( sv );
            	    nCs = mCrossSection.getCrossSection( csv, scale );

					// Orient it and add the vertecies to the mesh
            	    ori = orient( v + vinc );
            	    cCpos = mCarrier.getInterpolator()->getPosition( v + vinc );

            	    for( int i = 0; i < nCs.size(); ++i )
            	    {
            	        cVert = cCpos + ori * nCs[i];
            	        mb.addVertex( cVert.getX(), cVert.getY(), cVert.getZ() );
            	    }

            	    // While neither cross section is empty add the required triangles
            	    int i = 0;
            	    int j = 0;
            	    int s1 = pCs.size();
            	    int s2 = nCs.size();

					// Calculate the total length of the new cs
					pTLength = getTotalLengthSquared( pCs );
					nTLength = getTotalLengthSquared( nCs );
					pLength = (pCs[0] - pCs[(1) % s1]).length() / pTLength;
					nLength = (nCs[0] - nCs[(1) % s2]).length() / nTLength;
					pRTLength = 0.;
					nRTLength = 0.;

					//std::cout << "pTLength = " << pTLength << ", nTLength = " << nTLength << "\n";


            	    while( i < s1 && j < s2 )
            	    {
						//std::cout << "i = " << i << ", j = " << j << "\n";
						//std::cout << ( pCs[i] - nCs[( j + 1 ) % s2] ).lengthSquared() << std::endl;
						//std::cout << "p = " << pLength / pTLength << ", n = " << nLength / nTLength << "\n";
						//std::cout << "\n\ni = " << i << ", j = " << j <<std::endl;

						//if( nLength < 1E-6 ) std::cout << "Oh noess!\n";

						// They are almost the same, add both triangles
						//std::cout << "p = " << pLength + pRTLength << ", n = " << nLength + nRTLength << "\n";
						if( fabs(nLength + nRTLength - pLength - pRTLength) < 1E-9 ) {
            	            mb.addFace( pCsIndex + i, nCsIndex + ( ( j + 1 ) % s2 ), nCsIndex + j );
            	            ++j;
							nRTLength += nLength;
							nLength = (nCs[j] - nCs[(j + 1) % s2]).length() / nTLength;

							//std::cout << "Adding the point on the p.\n";
            	            mb.addFace( pCsIndex + i, pCsIndex + ( ( i + 1 ) % s1 ), nCsIndex + (j % s2) );
            	            ++i;
							pRTLength += pLength;
							pLength = (pCs[i] - pCs[(i + 1) % s1]).length() / pTLength;
						}
						else if( nLength + nRTLength < pLength + pRTLength ) {
							//std::cout << "Adding the point on the n.\n";
            	            mb.addFace( pCsIndex + i, nCsIndex + ( ( j + 1 ) % s2 ), nCsIndex + j );
            	            ++j;
							nRTLength += nLength;
							nLength = (nCs[j] - nCs[(j + 1) % s2]).length() / nTLength;
						}
						else {
							//std::cout << "Adding the point on the p.\n";
            	            mb.addFace( pCsIndex + i, pCsIndex + ( ( i + 1 ) % s1 ), nCsIndex + j );
            	            ++i;
							pRTLength += pLength;
							pLength = (pCs[i] - pCs[(i + 1) % s1]).length() / pTLength;
						}

//						if( nRTLength > pRTLength )
//            	        //if( ( pCs[i] - nCs[( j + 1 ) % s2] ).lengthSquared() > ( pCs[( i + 1 ) % s1] - nCs[j] ).lengthSquared() )
//            	        {
//							std::cout << "Adding the point on the p.\n";
//            	            mb.addFace( pCsIndex + i, pCsIndex + ( ( i + 1 ) % s1 ), nCsIndex + j );
//            	            ++i;
//							pLength = (pCs[i] - pCs[(i + 1) % s1]).lengthSquared() / pTLength;
//							pRTLength += pLength;
//            	        }
//            	        else
//            	        {
//							std::cout << "Adding the point on the n.\n";
//            	            mb.addFace( pCsIndex + i, nCsIndex + ( ( j + 1 ) % s2 ), nCsIndex + j );
//            	            ++j;
//							nLength = (nCs[j] - nCs[(j + 1) % s2]).lengthSquared() / nTLength;
//							nRTLength += nLength;
//            	        }

            	    }

            	    // Finish off any unjoined verticies
            	    while( i < s1 )
            	    {
            	        mb.addFace( pCsIndex + i, pCsIndex + ( ( i + 1 ) % s1 ), nCsIndex );
            	        ++i;
            	    }

            	    while( j < s2 )
            	    {
            	        mb.addFace( pCsIndex, nCsIndex + ( ( j + 1 ) % s2 ), nCsIndex + j );
            	        ++j;
            	    }

            	    // Update the previous cross section and indicies
            	    pCsIndex = nCsIndex;
            	    nCsIndex += nCs.size();
            	    pCs = nCs;
            	    // increment v
            	    v += vinc;
            	}
			}
        }

		double GeneralisedCylinder::getCrossSectionV( double v ) const
		{
			if (mCSDomain.size() == 0)
				return v;

			int i;

			for( i = 0; i < mCSDomain.size(); ++i)
			{
				if( v < mCSDomain[i].first )
					break;
			}

			if( i < 1 )
			{
				return mCSDomain[0].second;
			}
			else if( i >= mCSDomain.size() )
			{
				return mCSDomain.back().second;
			}
			else
			{
				return lerp<double,double>( mCSDomain[i-1].second, mCSDomain[i].second, (v - mCSDomain[i-1].first)/(mCSDomain[i].first - mCSDomain[i-1].first) );
			}
		}

		double GeneralisedCylinder::getScaleV( double v ) const
		{
			if (mScaleDomain.size() == 0)
				return v;

			int i;

			for( i = 0; i < mScaleDomain.size(); ++i)
			{
				if( v < mScaleDomain[i].first )
					break;
			}

			if( i < 1 )
			{
				return mScaleDomain[0].second;
			}
			else if( i >= mScaleDomain.size() )
			{
				return mScaleDomain.back().second;
			}
			else
			{
				return lerp<double,double>( mScaleDomain[i-1].second, mScaleDomain[i].second, (v - mScaleDomain[i-1].first)/(mScaleDomain[i].first - mScaleDomain[i-1].first) );
			}
		}

		double GeneralisedCylinder::getTotalLengthSquared( const std::vector< Vec3 > &polyLine ) const
		{
			double length = 0.;

			//std::cout << "in getTL size = " << polyLine.size() << "\n";

			int i;
			for( i = 1; i < polyLine.size(); ++i )
			{
				length += (polyLine[i - 1] - polyLine[i]).length();
				//std::cout << length << "\n";
			}
			length += (polyLine[i - 1] - polyLine[0]).length();

			return length;
		}

//boost::shared_ptr<Mesh> GeneralisedCylinder::createMesh(int n, int m) const
//{
//  fg::Mesh::MeshBuilder mb;
//
//    double vmin, vmax;
//    mCarrier.getInterpolator()->getDomain(vmin, vmax);
//  double vinc = (vmax-vmin) / ((double) m);
//  double uinc = (2.*M_PI) / ((double) n);
//  double v = 0.;
//  double u = 0.;
//    for (unsigned int j = 0; j <= m; ++j) {
//        for (unsigned int i = 0; i < n; ++i) {
//          Vec3 pos = getPosition(u,v);
//          mb.addVertex(pos.getX(),pos.getY(),pos.getZ());
//          u += uinc;
//        }
//      v += vinc;
//      u = 0.f;
//    }
//
//
//    for (unsigned int j = 0; j < m; ++j) {
//        for (unsigned int i = 0; i < n; ++i) {
//            mb.addFace((i+1)%(n)+j*n, i+(j+1)*n, i+j*n);
//            mb.addFace((i+1)%(n)+j*n, (i+1)%(n)+(j+1)*n, i+(j+1)*n);
//
//            //mb.addFace(i+j*n, i+(j+1)*n, (i+1)%(n)+j*n);
//            //mb.addFace(i+(j+1)*n, (i+1)%(n)+(j+1)*n, (i+1)%(n)+j*n);
//        }
//    }
//
//  return mb.createMesh();
//}
    }
}
