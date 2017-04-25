#include <cmath>
#include <algorithm>

#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DetectorDescription/Base/interface/DDutils.h"
#include "DetectorDescription/Core/interface/DDLogicalPart.h"
#include "DetectorDescription/Core/interface/DDSolid.h"
#include "DetectorDescription/Core/interface/DDMaterial.h"
#include "DetectorDescription/Core/interface/DDCurrentNamespace.h"
#include "DetectorDescription/Core/interface/DDSplit.h"
#include "Geometry/HGCalCommonData/plugins/DDHGCalTBModule.h"
#include "CLHEP/Units/GlobalPhysicalConstants.h"
#include "CLHEP/Units/GlobalSystemOfUnits.h"

#define EDM_ML_DEBUG

DDHGCalTBModule::DDHGCalTBModule() {
#ifdef EDM_ML_DEBUG
  std::cout << "DDHGCalTBModule info: Creating an instance" << std::endl;
#endif
}

DDHGCalTBModule::~DDHGCalTBModule() {}

void DDHGCalTBModule::initialize(const DDNumericArguments & nArgs,
				 const DDVectorArguments & vArgs,
				 const DDMapArguments & ,
				 const DDStringArguments & sArgs,
				 const DDStringVectorArguments &vsArgs){
  
  wafer         = vsArgs["WaferName"];
  covers        = vsArgs["CoverName"];
#ifdef EDM_ML_DEBUG
  std::cout << "DDHGCalTBModule: " << wafer.size() << " wafers" << std::endl;
  for (unsigned int i=0; i<wafer.size(); ++i)
    std::cout << "Wafer[" << i << "] " << wafer[i] << std::endl;
  std::cout << "DDHGCalTBModule: " << covers.size() << " covers" << std::endl;
  for (unsigned int i=0; i<covers.size(); ++i)
    std::cout << "Cover[" << i << "] " << covers[i] << std::endl;
#endif
  materials     = vsArgs["MaterialNames"];
  names         = vsArgs["VolumeNames"];
  thick         = vArgs["Thickness"];
  for (unsigned int i=0; i<materials.size(); ++i) {
    copyNumber.push_back(1);
  }
#ifdef EDM_ML_DEBUG
  std::cout << "DDHGCalTBModule: " << materials.size()
	    << " types of volumes" << std::endl;
  for (unsigned int i=0; i<names.size(); ++i)
    std::cout << "Volume [" << i << "] " << names[i] << " of thickness " 
	      << thick[i] << " filled with " << materials[i]
	      << " first copy number " << copyNumber[i] << std::endl;
#endif
  layers        = dbl_to_int(vArgs["Layers"]);
  layerThick    = vArgs["LayerThick"];
#ifdef EDM_ML_DEBUG
  std::cout << "DDHGCalTBModule: " << layers.size() << " blocks" <<std::endl;
  for (unsigned int i=0; i<layers.size(); ++i)
    std::cout << "Block [" << i << "] of thickness "  << layerThick[i] 
	      << " with " << layers[i] << " layers" << std::endl;
#endif
  layerType     = dbl_to_int(vArgs["LayerType"]);
  layerSense    = dbl_to_int(vArgs["LayerSense"]);
#ifdef EDM_ML_DEBUG
  std::cout << "DDHGCalTBModule: " << layerType.size() << " layers" 
	    << std::endl;
  for (unsigned int i=0; i<layerType.size(); ++i)
    std::cout << "Layer [" << i << "] with material type "  << layerType[i]
	      << " sensitive class " << layerSense[i] << std::endl;
#endif
  zMinBlock     = nArgs["zMinBlock"];
  rMaxFine      = nArgs["rMaxFine"];
  waferW        = nArgs["waferW"];
  waferGap      = nArgs["waferGap"];
  absorbW       = nArgs["absorberW"];
  absorbH       = nArgs["absorberH"];
  sectors       = (int)(nArgs["Sectors"]);
#ifdef EDM_ML_DEBUG
  std::cout << "DDHGCalTBModule: zStart " << zMinBlock << " rFineCoarse " 
	    << rMaxFine << " wafer width " << waferW << " gap among wafers "
	    << waferGap << " absorber width " << absorbW << " absorber height "
	    << absorbH << " sectors " << sectors << std::endl;
#endif
  slopeB        = vArgs["SlopeBottom"];
  slopeT        = vArgs["SlopeTop"];
  zFront        = vArgs["ZFront"];
  rMaxFront     = vArgs["RMaxFront"];
#ifdef EDM_ML_DEBUG
  std::cout << "DDHGCalTBModule: Bottom slopes " << slopeB[0] << ":" 
	    << slopeB[1] << " and " << slopeT.size() << " slopes for top" 
	    << std::endl;
  for (unsigned int i=0; i<slopeT.size(); ++i)
    std::cout << "Block [" << i << "] Zmin " << zFront[i] << " Rmax "
	      << rMaxFront[i] << " Slope " << slopeT[i] << std::endl;
#endif
  idNameSpace   = DDCurrentNamespace::ns();
#ifdef EDM_ML_DEBUG
  std::cout << "DDHGCalTBModule: NameSpace " << idNameSpace << std::endl;
#endif
}

////////////////////////////////////////////////////////////////////
// DDHGCalTBModule methods...
////////////////////////////////////////////////////////////////////

void DDHGCalTBModule::execute(DDCompactView& cpv) {
  
#ifdef EDM_ML_DEBUG
  std::cout << "==>> Constructing DDHGCalTBModule..." << std::endl;
#endif
  copies.clear();
  constructLayers (parent(), cpv);
#ifdef EDM_ML_DEBUG
  std::cout << copies.size() << " different wafer copy numbers" << std::endl;
#endif
  copies.clear();
#ifdef EDM_ML_DEBUG
  std::cout << "<<== End of DDHGCalTBModule construction ..." << std::endl;
#endif
}

void DDHGCalTBModule::constructLayers(DDLogicalPart module, 
				      DDCompactView& cpv) {
  
#ifdef EDM_ML_DEBUG
  std::cout << "DDHGCalTBModule test: \t\tInside Layers" << std::endl;
#endif
  double       zi(zMinBlock);
  int          laymin(0);
  for (unsigned int i=0; i<layers.size(); i++) {
    double  zo     = zi + layerThick[i];
    double  routF  = rMax(zi);
    int     laymax = laymin+layers[i];
    double  zz     = zi;
    double  thickTot(0);
    for (int ly=laymin; ly<laymax; ++ly) {
      int     ii     = layerType[ly];
      int     copy   = copyNumber[ii];
      double  rinB   = (layerSense[ly] == 0) ? (zo*slopeB[0]) : (zo*slopeB[1]);
      zz            += (0.5*thick[ii]);
      thickTot      += thick[ii];

      std::string name = "HGCal"+names[ii]+std::to_string(copy);
#ifdef EDM_ML_DEBUG
      std::cout << "DDHGCalTBModule test: Layer " << ly << ":" << ii 
		<< " Front " << zi << ", " << routF << " Back " << zo << ", " 
		<< rinB << " superlayer thickness " << layerThick[i] 
		<< std::endl;
#endif
      DDName matName(DDSplit(materials[ii]).first, 
		     DDSplit(materials[ii]).second);
      DDMaterial matter(matName);
      DDLogicalPart glog;
      if (layerSense[ly] == 0) {
	DDSolid solid = DDSolidFactory::box(DDName(name, idNameSpace),
					    absorbW, absorbH, 0.5*thick[ii]);
	glog = DDLogicalPart(solid.ddname(), matter, solid);
#ifdef EDM_ML_DEBUG
	std::cout << "DDHGCalTBModule test: " << solid.name() 
		  << " box of dimension " << absorbW << ":" << absorbH
		  << ":" << 0.5*thick[ii] << std::endl;
#endif
      } else {
	DDSolid solid = DDSolidFactory::tubs(DDName(name, idNameSpace), 
					     0.5*thick[ii], rinB, routF, 0.0,
					     CLHEP::twopi);
	glog = DDLogicalPart(solid.ddname(), matter, solid);
#ifdef EDM_ML_DEBUG
	std::cout << "DDHGCalTBModule test: " << solid.name()
		  << " Tubs made of " << matName << " of dimensions " << rinB 
		  << ", " << routF << ", " << 0.5*thick[ii] << ", 0.0, "
		  << CLHEP::twopi/CLHEP::deg << std::endl;
#endif
	positionSensitive(glog,layerSense[ly],rinB,routF,cpv);
      }
      DDTranslation r1(0,0,zz);
      DDRotation rot;
      cpv.position(glog, module, copy, r1, rot);
      ++copyNumber[ii];
#ifdef EDM_ML_DEBUG
      std::cout << "DDHGCalTBModule test: " << glog.name() << " number "
		<< copy << " positioned in " << module.name() << " at " << r1 
		<< " with " << rot << std::endl;
#endif
      zz += (0.5*thick[ii]);
    } // End of loop over layers in a block
    zi     = zo;
    laymin = laymax;
    if (fabs(thickTot-layerThick[i]) < 0.00001) {
    } else if (thickTot > layerThick[i]) {
      edm::LogError("HGCalGeom") << "Thickness of the partition " << layerThick[i]
				 << " is smaller than thickness " << thickTot
				 << " of all its components **** ERROR ****\n";
    } else if (thickTot < layerThick[i]) {
      edm::LogWarning("HGCalGeom") << "Thickness of the partition " 
				   << layerThick[i] << " does not match with "
				   << thickTot << " of the components\n";
    }
  }   // End of loop over blocks
}

double DDHGCalTBModule::rMax(double z) {
  double r(0);
#ifdef EDM_ML_DEBUG
  unsigned int ik(0);
#endif
  for (unsigned int k=0; k<slopeT.size(); ++k) {
    if (z < zFront[k]) break;
    r  = rMaxFront[k] + (z - zFront[k]) * slopeT[k];
#ifdef EDM_ML_DEBUG
    ik = k;
#endif
  }
#ifdef EDM_ML_DEBUG
  std::cout << "rMax : " << z << ":" << ik << ":" << r << std::endl;
#endif
  return r;
}

void DDHGCalTBModule::positionSensitive(DDLogicalPart& glog, int type,
					double rin, double rout, 
					DDCompactView& cpv) {
  double ww   = (waferW+waferGap);
  double dx   = 0.5*ww;
  double dy   = 3.0*dx*tan(30.0*CLHEP::deg);
  double rr   = 2.0*dx*tan(30.0*CLHEP::deg);
  int    ncol = (int)(2.0*rout/ww) + 1;
  int    nrow = (int)(rout/(ww*tan(30.0*CLHEP::deg))) + 1;
  int    incm(0), inrm(0), kount(0);
  double xc[6], yc[6];
#ifdef EDM_ML_DEBUG
  std::cout << glog.ddname() << " rout " << rout << " Row " << nrow 
	    << " Column " << ncol << std::endl; 
#endif
  for (int nr=-nrow; nr <= nrow; ++nr) {
    int inr = (nr >= 0) ? nr : -nr;
    for (int nc=-ncol; nc <= ncol; ++nc) {
      int inc = (nc >= 0) ? nc : -nc;
      if (inr%2 == inc%2) {
	double xpos = nc*dx;
	double ypos = nr*dy;
        xc[0] = xpos+dx; yc[0] = ypos-0.5*rr;
        xc[1] = xpos+dx; yc[1] = ypos+0.5*rr;
        xc[2] = xpos;    yc[2] = ypos+rr;
        xc[3] = xpos-dx; yc[3] = ypos+0.5*rr;
        xc[4] = xpos+dx; yc[4] = ypos-0.5*rr;
        xc[5] = xpos;    yc[5] = ypos-rr;
        bool cornerAll(true);
        for (int k=0; k<6; ++k) {
          double rpos = std::sqrt(xc[k]*xc[k]+yc[k]*yc[k]);
          if (rpos < rin || rpos > rout) cornerAll = false;
        }
	if (cornerAll) {
          double rpos = std::sqrt(xpos*xpos+ypos*ypos);
	  DDTranslation tran(xpos, ypos, 0.0);
	  DDRotation rotation;
	  int copy = inr*100 + inc;
	  if (nc < 0) copy += 10000;
	  if (nr < 0) copy += 100000;
	  DDName name;
	  if (type == 1) {
	    name = (rpos < rMaxFine) ? 
	      DDName(DDSplit(wafer[0]).first, DDSplit(wafer[0]).second) : 
	      DDName(DDSplit(wafer[1]).first, DDSplit(wafer[1]).second);
	  } else {
	    name = DDName(DDSplit(covers[type-2]).first, 
			  DDSplit(covers[type-2]).second); 
	  }
	  cpv.position(name, glog.ddname(), copy, tran, rotation);
	  if (inc > incm) incm = inc;
	  if (inr > inrm) inrm = inr;
	  kount++;
	  if (copies.count(copy) == 0 && type == 1)
	    copies.insert(copy);
#ifdef EDM_ML_DEBUG
	  std::cout << "DDHGCalTBModule: " << name << " number " << copy
		    << " positioned in " << glog.ddname() << " at " << tran 
		    << " with " << rotation << std::endl;
#endif
	}
      }
    }
  }
#ifdef EDM_ML_DEBUG
  std::cout << "DDHGCalTBModule: # of columns " << incm << " # of rows " 
	    << inrm << " and " << kount << " wafers for " << glog.ddname()
	    << std::endl;
#endif
}
