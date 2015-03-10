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

/*! \file moc.h
 *  Copyright (C) 2014-2015 Max-Planck-Society
 *  \author Martin Reinecke
 */

#ifndef HEALPIX_MOC_H
#define HEALPIX_MOC_H

#include "healpix_base.h"
#include "compress_utils.h"

template<typename I> class Moc
  {
  public:
    enum{maxorder=T_Healpix_Base<I>::order_max};

  private:
    rangeset<I> rs;

    static Moc fromNewRangeSet(const rangeset<I> &rngset)
      {
      Moc res;
      res.rs = rngset;
      return res;
      }

  public:
    const rangeset<I> &Rs() { return rs; }
    tsize maxOrder() const
      {
      I combo=0;
      for (tsize i=0; i<rs.nranges(); ++i)
        combo|=rs.ivbegin(i)|rs.ivend(i);
      return maxorder-(trailingZeros(combo)>>1);
      }
    Moc degradedToOrder (int order, bool keepPartialCells) const
      {
      int shift=2*(maxorder-order);
      I ofs=(I(1)<<shift)-1;
      I mask = ~ofs;
      I adda = keepPartialCells ? I(0) : ofs,
        addb = keepPartialCells ? ofs : I(0);
      rangeset<I> rs2;
      for (tsize i=0; i<rs.nranges(); ++i)
        {
        I a=(rs.ivbegin(i)+adda)&mask;
        I b=(rs.ivend  (i)+addb)&mask;
        if (b>a) rs2.append(a,b);
        }
      return fromNewRangeSet(rs2);
      }
    void addPixelRange (int order, I p1, I p2)
      {
      int shift=2*(maxorder-order);
      rs.add(p1<<shift,p2<<shift);
      }
    void appendPixelRange (int order, I p1, I p2)
      {
      int shift=2*(maxorder-order);
      rs.append(p1<<shift,p2<<shift);
      }
    void appendPixel (int order, I p)
      { appendPixelRange(order,p,p+1); }
    /*! Returns a new Moc that contains the union of this Moc and \a other. */
    Moc op_or (const Moc &other) const
      { return fromNewRangeSet(rs.op_or(other.rs)); }
    /*! Returns a new Moc that contains the intersection of this Moc and
        \a other. */
    Moc op_and (const Moc &other) const
      { return fromNewRangeSet(rs.op_and(other.rs)); }
    Moc op_xor (const Moc &other) const
      { return fromNewRangeSet(rs.op_xor(other.rs)); }
    /*! Returns a new Moc that contains all parts of this Moc that are not
        contained in \a other. */
    Moc op_andnot (const Moc &other) const
      { return fromNewRangeSet(rs.op_andnot(other.rs)); }
    /*! Returns the complement of this Moc. */
    Moc complement() const
      {
      rangeset<I> full; full.append(I(0),I(12)*(I(1)<<(2*maxorder)));
      return fromNewRangeSet(full.op_andnot(rs));
      }
    /** Returns \c true if \a other is a subset of this Moc, else \c false. */
    bool contains(const Moc &other) const
      { return rs.contains(other.rs); }
    /** Returns \c true if the intersection of this Moc and \a other is not
        empty. */
    bool overlaps(const Moc &other) const
      { return rs.overlaps(other.rs); }

    /** Returns a vector containing all HEALPix pixels (in ascending NUNIQ order)
        covered by this Moc. The result is well-formed in the sense that every
        pixel is given at its lowest possible HEALPix order. */
    std::vector<I> toUniq() const
      {
      std::vector<I> res;
      std::vector<std::vector<I> > buf(maxorder+1);
      for (tsize i=0; i<rs.nranges(); ++i)
        {
        I start=rs.ivbegin(i), end=rs.ivend(i);
        while(start!=end)
          {
          int logstep=std::min<int>(maxorder,trailingZeros(start)>>1);
          logstep=std::min(logstep,ilog2(end-start)>>1);
          buf[maxorder-logstep].push_back(start);
          start+=I(1)<<(2*logstep);
          }
        }
      for (int o=0; o<=maxorder; ++o)
        {
        I ofs=I(1)<<(2*o+2);
        int shift=2*(maxorder-o);
        for (tsize j=0; j<buf[o].size(); ++j)
          res.push_back((buf[o][j]>>shift)+ofs);
        }
      return res;
      }
#if 0
    std::vector<I> toUniq_old() const
      {
      std::vector<I> res;
      rangeset<I> r2(rs), r3;
      for (int o=0; o<=maxorder; ++o)
        {
        if (r2.empty()) return res;

        int shift = 2*(maxorder-o);
        I ofs=(I(1)<<shift)-1;
        I ofs2 = I(1)<<(2*o+2);
        r3.clear();
        for (tsize iv=0; iv<r2.nranges(); ++iv)
          {
          I a=(r2.ivbegin(iv)+ofs)>>shift,
            b=r2.ivend(iv)>>shift;
          r3.append(a<<shift, b<<shift);
          for (I i=a; i<b; ++i) res.push_back(i+ofs2);
          }
        if (!r3.empty())
          r2 = r2.op_andnot(r3);
        }
      return res;
      }
    static Moc fromUniq_bad (const std::vector<I> &vu)
      {
      struct pix_o
        {
        I pix; int o;
        pix_o(I pix_, int o_) : pix(pix_), o(o_) {}
        bool operator<(const pix_o &other) const { return pix>other.pix; }
        };
      using namespace std;
      rangeset<I> res;
      vector<queue<I> > buf(maxorder+1);
      for (tsize i=0; i<vu.size(); ++i)
        {
        int o=ilog2(vu[i]>>2)>>1;
        I pix=vu[i]-(I(1)<<(2*o+2));
        pix<<=2*(maxorder-o);
        buf[o].push(pix);
        }
      priority_queue<pix_o> q;
      for (int o=0; o<=maxorder; ++o)
        if (!buf[o].empty())
          {
          q.push(pix_o(buf[o].front(),o));
          buf[o].pop();
          }
      while(!q.empty())
        {
        I pix=q.top().pix;
        int o=q.top().o;
        q.pop();
        if (!buf[o].empty())
          {
          q.push(pix_o(buf[o].front(),o));
          buf[o].pop();
          }
        res.append(pix,pix+(I(1)<<(2*(maxorder-o))));
        }
      return fromNewRangeSet(res);
      }
#endif
    static Moc fromUniq (const std::vector<I> &vu)
      {
      rangeset<I> r, rtmp;
      int lastorder=0;
      int shift=2*maxorder;
      for (tsize i=0; i<vu.size(); ++i)
        {
        int order = ilog2(vu[i]>>2)>>1;
        if (order!=lastorder)
          {
          r=r.op_or(rtmp);
          rtmp.clear();
          lastorder=order;
          shift=2*(maxorder-order);
          }
        I pix = vu[i]-(I(1)<<(2*order+2));
        rtmp.append (pix<<shift,(pix+1)<<shift);
        }
      r=r.op_or(rtmp);
      return fromNewRangeSet(r);
      }

    std::vector<uint8> toCompressed() const
      {
      obitstream obs;
      interpol_encode(rs.data().begin(),rs.data().end(),obs);
      return obs.state();
      }
    static Moc fromCompressed(const std::vector<uint8> &data)
      {
      ibitstream ibs(data);
      std::vector<I> v;
      interpol_decode(v,ibs);
      Moc out;
      out.rs.setData(v);
      return out;
      }

    bool operator==(const Moc &other) const
      {
      if (this == &other)
        return true;
      return rs==other.rs;
      }

    tsize nranges() const
      { return rs.nranges(); }
  };

#endif
