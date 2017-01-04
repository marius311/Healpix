/*
 *  This file is part of Healpix_cxx.
 *
 *  Healpix_cxx is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Healpix_cxx is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Healpix_cxx; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  For more information about HEALPix, see http://healpix.sourceforge.net
 */

/*
 *  Healpix_cxx is being developed at the Max-Planck-Institut fuer Astrophysik
 *  and financially supported by the Deutsches Zentrum fuer Luft- und Raumfahrt
 *  (DLR).
 */

/*
 *  Copyright (C) 2017 Max-Planck-Society
 *  Author: Martin Reinecke
 */

#include <memory>
#include <fstream>
#include "xcomplex.h"
#include "paramfile.h"
#include "alm.h"
#include "alm_fitsio.h"
#include "fitshandle.h"
#include "lsconstants.h"
#include "announce.h"

using namespace std;

namespace {

class needlet_base
  {
  public:
    virtual int lmin (int band) const = 0;
    virtual int lmax (int band) const = 0;
    virtual std::vector<double> getBand(int band) const = 0;
  };


class needlet: public needlet_base
  {
  private:
    enum { npoints=1000 };
    std::vector<double> val;
    double B;

    double phi(double t) const
      {
      using namespace std;
      double pos=1.-2*B/(B-1.)*(t-1./B);
      double p2=(val.size()-1)*abs(pos);
      size_t ip2=size_t(p2);
      if (ip2>=val.size()-1) return (pos>0.) ? 1. : 0.;
      double delta=p2-ip2;
      double v=val[ip2] + delta*(val[ip2+1]-val[ip2]);
      return (pos>0) ? 0.5+v : 0.5-v;
      }
    double b2 (double xi) const
      { return phi(xi/B) - phi(xi); }

  public:
    needlet (double B_) : val(npoints), B(B_)
      {
      using namespace std;
      val[0]=0;
      double vold=exp(-1.);
      for (int i=1; i<npoints-1; ++i)
        {
        double p0=double(i)/(npoints-1);
        double v=exp(-1./(1-p0*p0));
        val[i]=val[i-1]+0.5*(vold+v);
        vold=v;
        }
      val[npoints-1]=val[npoints-2]+0.5*vold;
      double xnorm=.5/val[npoints-1];
      for (int i=0; i<npoints; ++i)
        val[i]*=xnorm;
      }

    virtual int lmin (int band) const
      { return int(std::pow(B,band-1)+1.+1e-10); }
    virtual int lmax (int band) const
      { return int(std::pow(B,band+1)-1e-10); }
    virtual std::vector<double> getBand(int band) const
      {
      using namespace std;
      double fct=pow(B,-band);
      vector<double> res(lmax(band)+1);
      for (int i=0; i<lmin(band); ++i) res[i] = 0;
      for (int i=lmin(band); i<int(res.size()); ++i)
        res[i]=sqrt(b2(i*fct));
      return res;
      }
  };

class sdw: public needlet_base
  {
  private:
    enum { npoints=1000 };
    std::vector<double> val;
    double B;

    double f_s2dw(double k)
      {
      if ((k<=1./B)||(k>=1.)) return 0;
      double t = (k - (1. / B)) * (2.0 * B / (B-1.)) - 1.;
      return exp(-2.0 / (1.0 - t*t)) / k;
      }
    double phi (double t) const
      {
      using namespace std;
      double p2= (t-1./B)*(val.size()-1)/(1.-1./B);
      int ip2=int(p2);
      if (ip2>=int(val.size()-1)) return 0;
      if (p2<=0.) return 1;
      double delta=p2-ip2;
      return 1.-(val[ip2] + delta*(val[ip2+1]-val[ip2]));
      }
    double b2 (double xi) const
      { return phi(xi/B) - phi(xi); }

  public:
    sdw (double B_) : val(npoints), B(B_)
      {
      using namespace std;
      val[0]=0;
      double vold=0.;
      for (int i=1; i<npoints-1; ++i)
        {
        double p0=1./B+double(i)/(npoints-1)*(1-1./B);
        double v=f_s2dw(p0);
        val[i]=val[i-1]+0.5*(v+vold);
        vold=v;
        }
      val[npoints-1]=val[npoints-2]+0.5*vold;
      double xnorm=1./val[npoints-1];
      for (int i=0; i<npoints; ++i)
        val[i]*=xnorm;
      }

    virtual int lmin (int band) const
      { return int(std::pow(B,band-1)+1.+1e-10); }
    virtual int lmax (int band) const
      { return int(std::pow(B,band+1)-1e-10); }
    virtual std::vector<double> getBand(int band) const
      {
      using namespace std;
      double fct=pow(B,-band);
      vector<double> res(lmax(band)+1);
      for (int i=0; i<lmin(band); ++i) res[i] = 0;
      for (int i=lmin(band); i<int(res.size()); ++i)
        res[i]=sqrt(b2(i*fct));
      return res;
      }
  };

class basak: public needlet_base
  {
  private:
    std::vector<int> peaks;

    void checkBand(int band) const
      {
      planck_assert((band>=0) && (band<int(peaks.size())), "illegal band");
      }

  public:
    basak (const std::vector<int> &peaks_) : peaks(peaks_)
      {
      planck_assert(peaks.size()>0, "empty peaks vector");
      planck_assert(peaks[0]>0,"first peak must be >0");
      for (tsize i=1; i<peaks.size(); ++i)
        planck_assert(peaks[i]>peaks[i-1],"peaks not strictly increasing");
      }

    virtual int lmin (int band) const
      { checkBand(band); return (band>0) ? peaks[band-1]+1 : 0; }
    virtual int lmax (int band) const
      {
      checkBand(band); return (band<int(peaks.size())-1) ?
        peaks[band+1]-1:peaks[band];
      }
    virtual std::vector<double> getBand(int band) const
      {
      using namespace std;
      double halfpi=0.5*M_PI;
      vector<double> res(lmax(band)+1);
      for (int i=0; i<lmin(band); ++i) res[i]=0.;
      if (band>0)
        for (int i=lmin(band); i<=peaks[band]; ++i)
          res[i]=cos(halfpi*(peaks[band]-i)/(peaks[band]-peaks[band-1]));
      if (band<int(peaks.size())-1)
        for (int i=peaks[band]; i<=lmax(band); ++i)
          res[i]=cos(halfpi*(i-peaks[band])/(peaks[band+1]-peaks[band]));
      return res;
      }
  };

bool fileExists(const string &name)
  {
  ifstream inp(name);
  return bool(inp);
  }

template<typename T> void needlet_tool (paramfile &params)
  {
  string infile = params.template find<string>("infile");
  string outfile = params.template find<string>("outfile");
  string mode = params.template find<string>("mode");
  planck_assert ((mode=="split") || (mode=="assemble"),"invalid mode");
  bool split = mode=="split";
  unique_ptr<needlet_base> needgen;
  string ntype = params.template find<string>("needlet_type");
  if (ntype=="basak")
    {
    vector<string> lps=tokenize(params.template find<string>("lpeak"),',');
    vector<int> lpeak;
    for (auto x:lps) lpeak.push_back(stringToData<int>(x));
    needgen.reset(new basak(lpeak));
    }
  else if (ntype=="needatool")
    needgen.reset(new needlet(params.template find<double>("B")));
  else if (ntype=="sdw")
    needgen.reset(new sdw(params.template find<double>("B")));
  else
    planck_fail("unknown needlet type");

  bool polarisation = params.template find<bool>("polarisation");
  if (!polarisation)
    {
    if (split)
      {
      int nlmax, nmmax;
      get_almsize(infile, nlmax, nmmax);
      Alm<xcomplex<T> > alm;
      read_Alm_from_fits(infile,alm,nlmax,nmmax);
      int hiband=0;
      while (needgen->lmin(hiband)<=nlmax) ++hiband;
      for (int i=0; i<hiband; ++i)
        {
        int lmax_t=min(nlmax,needgen->lmax(i)),
            mmax_t=min(nmmax, lmax_t);
        Alm<xcomplex<T>> atmp(lmax_t,mmax_t);
        for (int m=0; m<=mmax_t; ++m)
          for (int l=m; l<=lmax_t; ++l)
            atmp(l,m) = alm(l,m);
        atmp.ScaleL(needgen->getBand(i));
        write_Alm_to_fits (outfile+intToString(i,3)+".fits",atmp,lmax_t,mmax_t,
          planckType<T>());
        }
      }
    else
      {
      Alm<xcomplex<T> > alm;
      for (int i=0; fileExists(infile+intToString(i,3)+".fits"); ++i)
        {
        int nlmax, nmmax;
        get_almsize(infile+intToString(i,3)+".fits", nlmax, nmmax);
        Alm<xcomplex<T> > atmp;
        read_Alm_from_fits(infile+intToString(i,3)+".fits",atmp,nlmax,nmmax);
        atmp.ScaleL(needgen->getBand(i));
        for (int m=0; m<=alm.Mmax(); ++m)
          for (int l=m; l<=alm.Lmax(); ++l)
            atmp(l,m) += alm(l,m);
        atmp.swap(alm);
        write_Alm_to_fits (outfile,alm,alm.Lmax(),alm.Mmax(), planckType<T>());
        }
      }
    }
  else
    {
    planck_fail("polarisation not yet supported");
    }
  }

} // unnamed namespace

int needlet_tool (int argc, const char **argv)
  {
  module_startup ("needlet_tool", argc, argv);
  paramfile params (getParamsFromCmdline(argc,argv));

  bool dp = params.find<bool> ("double_precision",false);
  dp ? needlet_tool<double>(params) : needlet_tool<float>(params);

  return 0;
  }
