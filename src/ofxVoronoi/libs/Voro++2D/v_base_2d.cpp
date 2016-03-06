// Voro++, a 2D and 3D cell-based Voronoi library
//
// Author   : Chris H. Rycroft (LBL / UC Berkeley)
// Email    : chr@alum.mit.edu
// Date     : August 30th 2011

/** \file v_base_2d.cc
 * \brief Function implementations for the base 2D Voronoi container class. */
//#include <stdio.h>

using namespace std;
#include "v_base_2d.h"
#include "config.h"

namespace voro {
    
    
    

/** This function is called during container construction. The routine scans
 * all of the worklists in the wl[] array. For a given worklist of blocks
 * labeled \f$w_1\f$ to \f$w_n\f$, it computes a sequence \f$r_0\f$ to
 * \f$r_n\f$ so that $r_i$ is the minimum distance to all the blocks
 * \f$w_{j}\f$ where \f$j>i\f$ and all blocks outside the worklist. The values
 * of \f$r_n\f$ is calculated first, as the minimum distance to any block in
 * the shell surrounding the worklist. The \f$r_i\f$ are then computed in
 * reverse order by considering the distance to \f$w_{i+1}\f$. */
voro_base_2d::voro_base_2d(int nx_,int ny_,double boxx_,double boxy_) :
	nx(nx_), ny(ny_), nxy(nx_*ny_), boxx(boxx_), boxy(boxy_),
	xsp(1/boxx_), ysp(1/boxy_), mrad(new double[wl_hgridsq_2d*wl_seq_length_2d]) {
	const unsigned int b1=1<<21,b2=1<<22,b3=1<<24,b4=1<<25;
	const double xstep=boxx/wl_fgrid_2d,ystep=boxy/wl_fgrid_2d;
	int i,j,lx,ly,q;
	unsigned int f,*e=const_cast<unsigned int*> (wl);
	double xlo,ylo,xhi,yhi,minr,*radp=mrad;
	for(ylo=0,yhi=ystep,ly=0;ly<wl_hgrid_2d;ylo=yhi,yhi+=ystep,ly++) {
		for(xlo=0,xhi=xstep,lx=0;lx<wl_hgrid_2d;xlo=xhi,xhi+=xstep,lx++) {
			minr=large_number;
			for(q=e[0]+1;q<wl_seq_length_2d;q++) {
				f=e[q];
				i=(f&127)-64;
				j=(f>>7&127)-64;
				if((f&b2)==b2) {
					compute_minimum(minr,xlo,xhi,ylo,yhi,i-1,j);
					if((f&b1)==0) compute_minimum(minr,xlo,xhi,ylo,yhi,i+1,j);
				} else if((f&b1)==b1) compute_minimum(minr,xlo,xhi,ylo,yhi,i+1,j);
				if((f&b4)==b4) {
					compute_minimum(minr,xlo,xhi,ylo,yhi,i,j-1);
					if((f&b3)==0) compute_minimum(minr,xlo,xhi,ylo,yhi,i,j+1);
				} else if((f&b3)==b3) compute_minimum(minr,xlo,xhi,ylo,yhi,i,j+1);
			}
			q--;
			while(q>0) {
				radp[q]=minr;
				f=e[q];
				i=(f&127)-64;
				j=(f>>7&127)-64;
				compute_minimum(minr,xlo,xhi,ylo,yhi,i,j);
				q--;
			}
			*radp=minr;
			e+=wl_seq_length_2d;
			radp+=wl_seq_length_2d;
		}
	}
}

/** Computes the minimum distance from a subregion to a given block. If this distance
 * is smaller than the value of minr, then it passes
 * \param[in,out] minr a pointer to the current minimum distance. If the distance
 *                     computed in this function is smaller, then this distance is
 *                     set to the new one.
 * \param[out] (xlo,ylo) the lower coordinates of the subregion being
 *                       considered.
 * \param[out] (xhi,yhi) the upper coordinates of the subregion being
 *                       considered.
 * \param[in] (ti,tj) the coordinates of the block. */
void voro_base_2d::compute_minimum(double &minr,double &xlo,double &xhi,double &ylo,double &yhi,int ti,int tj) {
	double radsq,temp;
	if(ti>0) {temp=boxx*ti-xhi;radsq=temp*temp;}
	else if(ti<0) {temp=xlo-boxx*(1+ti);radsq=temp*temp;}
	else radsq=0;

	if(tj>0) {temp=boxy*tj-yhi;radsq+=temp*temp;}
	else if(tj<0) {temp=ylo-boxy*(1+tj);radsq+=temp*temp;}

	if(radsq<minr) minr=radsq;
}

/** Checks to see whether "%n" appears in a format sequence to determine
 * whether neighbor information is required or not.
 * \param[in] format the format string to check.
 * \return True if a "%n" is found, false otherwise. */
bool voro_base_2d::contains_neighbor(const char *format) {
	char *fmp=(const_cast<char*>(format));

	// Check to see if "%n" appears in the format sequence
	while(*fmp!=0) {
		if(*fmp=='%') {
			fmp++;
			if(*fmp=='n') return true;
			else if(*fmp==0) return false;
		}
		fmp++;
	}

	return false;
}

/*bool voro_base_2d::contains_neighbor_global(const char *format) {
	        char *fmp=(const_cast<char*>(format));

		        // Check to see if "%N" appears in the format sequence
		        while(*fmp!=0) {
				if(*fmp=='%') {
		                        fmp++;
					if(*fmp=='N') return true;
					else if(*fmp==0) return false;
				}   
				fmp++;
			}   

		        return false;
}
void voro_base_2d::add_globne_info(int pid, int *nel, int length){
	for(int i=0;i<length;i++){
		if(nel[i]>=0){
			globne[((totpar*pid)+nel[i])/32] |= (unsigned int)(1 << (((totpar*pid)+nel[i])%32));
		}
	}

}
void voro_base_2d::print_globne(FILE *fp){
	int j=0;
	fprintf(fp, "Global neighbor info: Format \n [Particle-ID] \t [Neighbors] \n [Particle-ID] \t [Neighbors]");
	for(int i=0; i<totpar; i++){
		fprintf(fp,"\n %d \t",i);
		for(j=0;j<totpar;j++){
			if((globne[(((i*totpar)+j)/32)] & (unsigned int)(1 << (((i*totpar)+j)%32))) != 0){
				fprintf(fp,"%d \t",j);
			}
		}
	}
}*/
    
    
    //wl_seq_length_2d*wl_hgridsq_2d
    const unsigned int voro_base_2d::wl[]={
    //const unsigned int voro_base_2d::wl[]={
        39,0x203f,0x1fc0,0x1fbf,0x20bf,0x20c0,0x2041,0x1fc1,0x1f40,0x1f3f,0x1fbe,0x203e,0x20c1,0x20be,0x1f41,0x1f3e,0x213f,0x2140,0x2042,0x1fc2,0x1ec0,0x1ebf,0x1fbd,0x203d,0x2141,0x20c2,0x1f42,0x1ec1,0x213e,0x20bd,0x1f3d,0x1ebe,0x2142,0x1ec2,0x213d,0x21bf,0x21c0,0x2043,0x1fc3,0x1ebd,0x3001e3f,0x3001e40,0x601fbc,0x60203c,0x21be,0x21c1,0x20c3,0x1f43,0x3001e41,0x3001e3e,0x601f3c,0x6020bc,0x10021bd,0x10021c2,0x1202143,0x201ec3,0x3201e42,0x3001e3d,0x3601ebc,0x160213c,0x160223f,0x1202240,0x1202044,0x3201fc4,
        36,0x1fc0,0x203f,0x1fbf,0x1fc1,0x2041,0x20c0,0x20bf,0x1f40,0x1f3f,0x1fbe,0x203e,0x20c1,0x1f41,0x20be,0x1f3e,0x1fc2,0x2042,0x2140,0x213f,0x20c2,0x2141,0x1ec0,0x1ebf,0x1f42,0x1ec1,0x1fbd,0x203d,0x213e,0x20bd,0x1ebe,0x1f3d,0x2142,0x1ec2,0x1fc3,0x2043,0x20c3,0x10021c0,0x10021bf,0x213d,0x1ebd,0x3001e3f,0x3001e40,0x1f43,0x10021c1,0x10021be,0x60203c,0x601fbc,0x3001e41,0x3001e3e,0x601f3c,0x6020bc,0x2143,0x12021c2,0x1ec3,0x3201e42,0x10021bd,0x160213c,0x3001e3d,0x3601ebc,0x201fc4,0x202044,0x12020c4,0x3201f44,
        37,0x1fc0,0x1fbf,0x203f,0x2041,0x1fc1,0x20c0,0x20bf,0x1f40,0x1f3f,0x20c1,0x1f41,0x1fbe,0x203e,0x20be,0x1f3e,0x1fc2,0x2042,0x20c2,0x2140,0x213f,0x1f42,0x1ec0,0x1ebf,0x2141,0x1ec1,0x1fbd,0x203d,0x213e,0x20bd,0x1ebe,0x1f3d,0x2142,0x2043,0x1fc3,0x1ec2,0x1f43,0x20c3,0x10021c0,0x10021bf,0x10021c1,0x213d,0x3001e40,0x3001e3f,0x1ebd,0x3001e41,0x10021be,0x60203c,0x601fbc,0x3001e3e,0x2143,0x12021c2,0x1ec3,0x3201e42,0x6020bc,0x601f3c,0x10021bd,0x202044,0x201fc4,0x12020c4,0x3201f44,0x3001e3d,0x3601ebc,0x160213c,
        38,0x1fc0,0x1fbf,0x203f,0x2041,0x1fc1,0x20c0,0x20bf,0x1f40,0x20c1,0x1f3f,0x1f41,0x1fbe,0x203e,0x2042,0x1fc2,0x20be,0x1f3e,0x20c2,0x1f42,0x2140,0x213f,0x2141,0x1ec0,0x1ebf,0x1ec1,0x1fbd,0x203d,0x213e,0x2142,0x2043,0x1fc3,0x1ebe,0x1ec2,0x20bd,0x1f3d,0x20c3,0x1f43,0x21c0,0x21bf,0x21c1,0x213d,0x3001e40,0x3001e3f,0x3001e41,0x1ebd,0x2143,0x10021c2,0x10021be,0x1ec3,0x3201e42,0x3601e3e,0x601fbc,0x60203c,0x6020bc,0x3601f3c,0x201fc4,0x202044,0x12020c4,0x3201f44,0x10021bd,0x160213c,0x1402240,0x12021c3,
        36,0x203f,0x1fc0,0x1fbf,0x20bf,0x20c0,0x2041,0x1fc1,0x203e,0x1fbe,0x1f3f,0x1f40,0x20c1,0x20be,0x1f41,0x1f3e,0x213f,0x2140,0x2042,0x1fc2,0x2141,0x20c2,0x203d,0x1fbd,0x213e,0x20bd,0x1ebf,0x1ec0,0x1f42,0x1ec1,0x1f3d,0x1ebe,0x2142,0x213d,0x21bf,0x21c0,0x21c1,0x202043,0x201fc3,0x1ec2,0x1ebd,0x601fbc,0x60203c,0x21be,0x2020c3,0x201f43,0x3001e40,0x3001e3f,0x6020bc,0x601f3c,0x3001e3e,0x3001e41,0x21c2,0x1202143,0x21bd,0x160213c,0x201ec3,0x3201e42,0x601ebc,0x3601e3d,0x100223f,0x1002240,0x1202241,0x160223e,
        40,0x203f,0x1fc0,0x1fbf,0x2041,0x20c0,0x20bf,0x1fc1,0x20c1,0x203e,0x1fbe,0x1f3f,0x1f40,0x1f41,0x20be,0x1f3e,0x2042,0x1fc2,0x2140,0x213f,0x2141,0x20c2,0x1f42,0x1ec0,0x1ebf,0x1fbd,0x203d,0x213e,0x20bd,0x1ec1,0x1ebe,0x1f3d,0x2142,0x2043,0x1fc3,0x1ec2,0x21c0,0x21bf,0x213d,0x21c1,0x20c3,0x1f43,0x21be,0x1ebd,0x3001e3f,0x3001e40,0x601fbc,0x60203c,0x6020bc,0x3001e41,0x3601e3e,0x3601f3c,0x21c2,0x1002143,0x201ec3,0x3201e42,0x10021bd,0x160213c,0x160223f,0x1002240,0x1202241,0x202044,0x3201fc4,0x12020c4,
        40,0x1fc0,0x203f,0x1fbf,0x2041,0x1fc1,0x20c0,0x20bf,0x20c1,0x1f40,0x1f3f,0x1fbe,0x203e,0x1f41,0x20be,0x2042,0x1fc2,0x1f3e,0x2140,0x213f,0x20c2,0x2141,0x1f42,0x1ec0,0x1ebf,0x1ec1,0x213e,0x203d,0x1fbd,0x20bd,0x2142,0x1ebe,0x1f3d,0x2043,0x1fc3,0x1ec2,0x20c3,0x21c0,0x21bf,0x21c1,0x213d,0x1f43,0x21be,0x1ebd,0x3001e3f,0x3001e40,0x3001e41,0x2143,0x2021c2,0x60203c,0x601fbc,0x6020bc,0x3601e3e,0x201ec3,0x3201e42,0x3601f3c,0x10021bd,0x160213c,0x202044,0x3201fc4,0x12020c4,0x1002240,0x160223f,0x1202241,
        39,0x1fc0,0x203f,0x1fbf,0x2041,0x1fc1,0x20c0,0x20bf,0x20c1,0x1f40,0x1f3f,0x1f41,0x203e,0x1fbe,0x2042,0x1fc2,0x20be,0x20c2,0x2140,0x213f,0x2141,0x1f3e,0x1f42,0x1ec0,0x1ebf,0x1ec1,0x213e,0x2142,0x203d,0x1fbd,0x20bd,0x2043,0x1fc3,0x1ebe,0x1f3d,0x1ec2,0x20c3,0x1f43,0x21c0,0x21bf,0x21c1,0x213d,0x21be,0x2143,0x10021c2,0x3001e40,0x3001e3f,0x3001e41,0x1ebd,0x601fbc,0x60203c,0x1ec3,0x3201e42,0x3601e3e,0x16020bc,0x3601f3c,0x202044,0x201fc4,0x12020c4,0x3201f44,0x16021bd,0x1202240,0x160223f,0x12021c3,
        37,0x203f,0x1fbf,0x1fc0,0x20c0,0x20bf,0x2041,0x1fc1,0x203e,0x1fbe,0x20c1,0x20be,0x1f3f,0x1f40,0x1f41,0x1f3e,0x213f,0x2140,0x2141,0x2042,0x1fc2,0x213e,0x203d,0x1fbd,0x20c2,0x20bd,0x1ebf,0x1ec0,0x1f42,0x1ec1,0x1f3d,0x1ebe,0x2142,0x21c0,0x21bf,0x213d,0x21be,0x21c1,0x202043,0x201fc3,0x2020c3,0x1ec2,0x60203c,0x601fbc,0x1ebd,0x6020bc,0x201f43,0x3001e40,0x3001e3f,0x601f3c,0x21c2,0x1202143,0x21bd,0x160213c,0x3001e41,0x3001e3e,0x201ec3,0x1002240,0x100223f,0x1202241,0x160223e,0x601ebc,0x3601e3d,0x3201e42,
        40,0x203f,0x1fc0,0x1fbf,0x20c0,0x20bf,0x2041,0x1fc1,0x20c1,0x203e,0x1fbe,0x1f3f,0x1f40,0x20be,0x1f41,0x2140,0x213f,0x1f3e,0x2042,0x1fc2,0x2141,0x20c2,0x213e,0x203d,0x1fbd,0x20bd,0x1f42,0x1ec0,0x1ebf,0x1ec1,0x2142,0x1f3d,0x1ebe,0x21c0,0x21bf,0x213d,0x21c1,0x2043,0x1fc3,0x20c3,0x1ec2,0x21be,0x1f43,0x1ebd,0x601fbc,0x60203c,0x6020bc,0x21c2,0x1002143,0x3001e40,0x3001e3f,0x3001e41,0x3601f3c,0x10021bd,0x160213c,0x3601e3e,0x201ec3,0x3201e42,0x1002240,0x160223f,0x1202241,0x202044,0x3201fc4,0x12020c4,
        39,0x203f,0x1fc0,0x1fbf,0x2041,0x20c0,0x20bf,0x1fc1,0x20c1,0x203e,0x1f40,0x1f3f,0x1fbe,0x20be,0x1f41,0x2042,0x1fc2,0x2140,0x213f,0x2141,0x20c2,0x1f3e,0x213e,0x1f42,0x1ec0,0x1ebf,0x203d,0x1fbd,0x20bd,0x2142,0x1ec1,0x1ebe,0x1f3d,0x21c0,0x21bf,0x2043,0x1fc3,0x20c3,0x21c1,0x213d,0x1ec2,0x1f43,0x21be,0x21c2,0x2143,0x1ebd,0x3001e3f,0x3001e40,0x3201e41,0x60203c,0x601fbc,0x6020bc,0x10021bd,0x3601f3c,0x3601e3e,0x3201ec3,0x3201fc4,0x202044,0x12020c4,0x1002240,0x160223f,0x160213c,0x1202241,0x12021c3,
        39,0x1fc0,0x203f,0x1fbf,0x2041,0x1fc1,0x20c0,0x20bf,0x20c1,0x1f40,0x1f3f,0x1f41,0x203e,0x1fbe,0x20be,0x2042,0x1fc2,0x20c2,0x2140,0x213f,0x2141,0x1f3e,0x1f42,0x213e,0x2142,0x1ec0,0x1ebf,0x1ec1,0x203d,0x1fbd,0x20bd,0x2043,0x1fc3,0x20c3,0x21c0,0x21bf,0x21c1,0x1ebe,0x1f3d,0x1ec2,0x1f43,0x213d,0x21be,0x2143,0x21c2,0x3001e40,0x3001e3f,0x3001e41,0x1ebd,0x601fbc,0x60203c,0x16020bc,0x201ec3,0x3201fc4,0x202044,0x12020c4,0x12021c3,0x16021bd,0x1002240,0x160223f,0x1202241,0x3601e3e,0x3601f3c,0x3201e42,
        38,0x203f,0x1fbf,0x1fc0,0x20c0,0x20bf,0x2041,0x1fc1,0x203e,0x20c1,0x1fbe,0x20be,0x1f3f,0x1f40,0x2140,0x213f,0x1f41,0x1f3e,0x2141,0x213e,0x2042,0x1fc2,0x20c2,0x203d,0x1fbd,0x20bd,0x1ebf,0x1ec0,0x1f42,0x2142,0x21c0,0x21bf,0x1f3d,0x213d,0x1ec1,0x1ebe,0x21c1,0x21be,0x2043,0x1fc3,0x20c3,0x1ec2,0x60203c,0x601fbc,0x6020bc,0x1ebd,0x21c2,0x202143,0x201f43,0x21bd,0x160213c,0x3601f3c,0x3001e3f,0x3001e40,0x3001e41,0x3601e3e,0x100223f,0x1002240,0x1202241,0x160223e,0x201ec3,0x3201e42,0x2202044,0x12021c3,
        39,0x203f,0x1fc0,0x1fbf,0x20c0,0x20bf,0x2041,0x1fc1,0x20c1,0x203e,0x1fbe,0x20be,0x1f40,0x1f3f,0x2140,0x213f,0x1f41,0x2141,0x2042,0x1fc2,0x20c2,0x1f3e,0x213e,0x203d,0x1fbd,0x20bd,0x1f42,0x2142,0x1ec0,0x1ebf,0x1ec1,0x21c0,0x21bf,0x1f3d,0x1ebe,0x213d,0x21c1,0x21be,0x2043,0x1fc3,0x20c3,0x1ec2,0x1f43,0x21c2,0x202143,0x60203c,0x601fbc,0x6020bc,0x1ebd,0x3001e3f,0x3001e40,0x21bd,0x160213c,0x3601f3c,0x3201e41,0x3601e3e,0x1002240,0x100223f,0x1202241,0x160223e,0x3201ec3,0x1202044,0x3201fc4,0x12021c3,
        39,0x203f,0x1fc0,0x1fbf,0x20c0,0x20bf,0x2041,0x1fc1,0x20c1,0x203e,0x1fbe,0x20be,0x1f40,0x1f3f,0x1f41,0x2140,0x213f,0x2141,0x2042,0x1fc2,0x20c2,0x1f3e,0x213e,0x1f42,0x2142,0x203d,0x1fbd,0x20bd,0x1ec0,0x1ebf,0x1ec1,0x21c0,0x21bf,0x21c1,0x2043,0x1fc3,0x20c3,0x1f3d,0x1ebe,0x213d,0x21be,0x1ec2,0x1f43,0x21c2,0x2143,0x60203c,0x601fbc,0x6020bc,0x1ebd,0x3001e3f,0x3001e40,0x3201e41,0x10021bd,0x160223f,0x1002240,0x1202241,0x12021c3,0x3201ec3,0x202044,0x3201fc4,0x12020c4,0x3601f3c,0x3601e3e,0x160213c,
        39,0x203f,0x1fc0,0x2041,0x20c0,0x1fbf,0x20bf,0x1fc1,0x20c1,0x203e,0x1f40,0x1f3f,0x1fbe,0x20be,0x2140,0x2042,0x1f41,0x1fc2,0x20c2,0x213f,0x2141,0x1f3e,0x213e,0x1f42,0x2142,0x203d,0x1fbd,0x1ec0,0x1ebf,0x1ec1,0x20bd,0x21c0,0x21bf,0x2043,0x1fc3,0x20c3,0x21c1,0x1f3d,0x1ebe,0x1ec2,0x1f43,0x213d,0x21be,0x21c2,0x2143,0x3001e40,0x3001e3f,0x1ebd,0x60203c,0x601fbc,0x16020bc,0x16021bd,0x1002240,0x160223f,0x1202241,0x3001e41,0x201ec3,0x202044,0x3201fc4,0x12020c4,0x12021c3,0x3601f3c,0x3601e3e,0x3201e42
    };
	
//#include "v_base_wl_2d.cpp"
}
