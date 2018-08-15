#include <memory.h>
#include "periodic_table.h"

namespace imago
{
    AtomSymbolMap AtomMap;

	bool AtomSymbolMap::lookup(const std::string& str) const
	{
		char c_str[4] = {0};
		strcpy_s(c_str, 3, str.c_str());
		return SymbolMap[*(unsigned short*)c_str] != 0;
	}

	void AtomSymbolMap::insert(const std::string& element)
	{
		Elements.push_back(element);
		char c_str[4] = {0};
		strcpy_s(c_str, 3, element.c_str());
		SymbolMap[*(unsigned short*)(c_str)] = Elements.size();
	}

    AtomSymbolMap::AtomSymbolMap()
    {        
        memset(SymbolMap, 0, sizeof(SymbolMap));
		insert("H");
		insert("He");
		insert("Li");
		insert("Be");
		insert("B");
		insert("C");
		insert("N");
		insert("O");
		insert("F");
		insert("Ne");
		insert("Na");
		insert("Mg");
		insert("Al");
		insert("Si");
		insert("P");
		insert("S");
		insert("Cl");
		insert("Ar");
		insert("K");
		insert("Ca");
		insert("Sc");
		insert("Ti");
		insert("V");
		insert("Cr");
		insert("Mn");
		insert("Fe");
		insert("Co");
		insert("Ni");
		insert("Cu");
		insert("Zn");
		insert("Ga");
		insert("Ge");
		insert("As");
		insert("Se");
		insert("Br");
		insert("Kr");
		insert("Rb");
		insert("Sr");
		insert("Y");
		insert("Zr");
		insert("Nb");
		insert("Mo");
		insert("Tc");
		insert("Ru");
		insert("Rh");
		insert("Pd");
		insert("Ag");
		insert("Cd");
		insert("In");
		insert("Sn");
		insert("Sb");
		insert("Te");
		insert("I");
		insert("Xe");
		insert("Cs");
		insert("Ba");
		insert("La");
		insert("Ce");
		insert("Pr");
		insert("Nd");
		insert("Pm");
		insert("Sm");
		insert("Eu");
		insert("Gd");
		insert("Tb");
		insert("Dy");
		insert("Ho");
		insert("Er");
		insert("Tm");
		insert("Yb");
		insert("Lu");
		insert("Hf");
		insert("Ta");
		insert("W");
		insert("Re");
		insert("Os");
		insert("Ir");
		insert("Pt");
		insert("Au");
		insert("Hg");
		insert("Tl");
		insert("Pb");
		insert("Bi");
		insert("Po");
		insert("At");
		insert("Rn");
		insert("Fr");
		insert("Ra");
		insert("Ac");
		insert("Th");
		insert("Pa");
		insert("U");
		insert("Np");
		insert("Pu");
		insert("Am");
		insert("Cm");
		insert("Bk");
		insert("Cf");
		insert("Es");
		insert("Fm");
		insert("Md");
		insert("No");
		insert("Lr");
		insert("Rf");
		insert("Df");
		insert("Sg");
		insert("Bh");
		insert("Hn");
		insert("Mt");
    }
}

