#pragma once


class Kernel
{
private: enum { LookupTableResolution = 100001};
public: Kernel():_h(1.0f),_halfH(_h * 0.5f){}
public: ~Kernel(){}

public: void SetH(const float h) { _h = h; _halfH = h * 0.5f; }

public: inline const float W_Cubic(const float & r, const float & h);
public: inline const float WG_Cubic(const float & r, const float & h);
public: inline const float W_Poly6(const float & r, const float & h);
public: inline const float WG_Poly6(const float & r, const float & h);
//public: inline const float W_Spikey(const float & r, const float & h);
public: inline const float WL_Viscosity(const float & r, const float & h);
		
private: float LookUpW_Cubic[LookupTableResolution];
private: float LookUpWG_Cubic[LookupTableResolution];
private: float LookUpWL_Viscosity[LookupTableResolution];

private: float _h;
private: float _halfH;
};

