#include "boost/foreach.hpp"
#include "boost/tuple/tuple.hpp"
#include <map>
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <deque>

#include "character_recognizer.h"
#include "segment.h"
#include "exception.h"
#include "scanner.h"
#include "png_loader.h"
#include "segmentator.h"
#include "stl_fwd.h"
#include "thin_filter2.h"
#include "image_utils.h"

using namespace imago;
const std::string CharacterRecognizer::upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string CharacterRecognizer::lower = "abcdefghijklmnopqrstuvwxyz";
const std::string CharacterRecognizer::digits = "0123456789";

CharacterRecognizer::CharacterRecognizer( int k ) : _loaded(false), _k(k)
{
   _mapping.resize(255, -1);
   _loadBuiltIn();
}

CharacterRecognizer::CharacterRecognizer( int k, const std::string &filename) :
   _loaded(false), _k(k)
{
   _mapping.resize(255, -1);
   _loadFromFile(filename);
}

CharacterRecognizer::~CharacterRecognizer()
{
}

double CharacterRecognizer::_compareDescriptors( const std::vector<double> &d1,
                                                 const std::vector<double> &d2 )
{
   int s = (int)std::min(d1.size(), d2.size());
   double d = 0;
   double r;
   double weight = 1;

   for (int i = 0; i < s; i++)
   {
      r = d1[i] - d2[i];

      if (i < s / 2)
      {
         if (i % 2)
            weight = 2.5;
         else
            weight = 3.5;
      }
      else
      {
         if (i % 2)
            weight = 0.9;
         else
            weight = 0.3;
      }
      d += fabs(weight * r); // * r;
   }

   return d;
}

double CharacterRecognizer::_compareFeatures( const SymbolFeatures &f1,
                                              const SymbolFeatures &f2 )
{
   double d = _compareDescriptors(f1.descriptors, f2.descriptors);

   if (f1.inner_contours_count == -1 || f2.inner_contours_count == -1)
      return sqrt(d);

   if (f1.inner_contours_count != f2.inner_contours_count)
      return DBL_MAX;
   
   for (int i = 0; i < f1.inner_contours_count; i++)
      if (f1.inner_descriptors[i].size() != 0 &&
          f2.inner_descriptors[i].size() != 0)
         d += _compareDescriptors(f1.inner_descriptors[i],
                                  f2.inner_descriptors[i]);

   return sqrt(d);
}

char CharacterRecognizer::recognize( const Segment &seg,
                                     const std::string &candidates,
                                     double *dist ) const
{
   seg.initFeatures(_count);
   return recognize(seg.getFeatures(), candidates, dist);
}

char CharacterRecognizer::recognize( const SymbolFeatures &features,
                                     const std::string &candidates,
                                     double *dist ) const
{
   if (!_loaded)
      throw OCRException("Font is not loaded");
   double d;

   std::vector<boost::tuple<char, int, double> > nearest;
   nearest.reserve(_k);

   BOOST_FOREACH( char c, candidates )
   {
      int ind = _mapping[c];
      const SymbolClass &cls = _classes[ind];
      for (int i = 0; i < (int)cls.shapes.size(); i++)
      {
         d = _compareFeatures(features, cls.shapes[i]);

         if ((int)nearest.size() < _k)
            nearest.push_back(boost::make_tuple(c, i, d));
         else
         {
            double far = boost::get<2>(nearest[0]), f;
            int far_ind = 0;
            for (int j = 1; j < _k; j++)
            {
               if ((f = boost::get<2>(nearest[j])) > far)
               {
                  far = f;
                  far_ind = j;
               }
            }

            if (d < far)
               nearest[far_ind] = boost::make_tuple(c, i, d);
         }
      }
   }

   typedef boost::tuple<char, int, double> NearestType;
   typedef std::map<char, boost::tuple<int, double> > ResultsMapType;
   ResultsMapType results;
   ResultsMapType::iterator it;
   int max = 1, x;
   BOOST_FOREACH(const NearestType &t, nearest)
   {
      char c = boost::get<0>(t);
      it = results.find(c);
      if (it == results.end())
      {
         results.insert(
            std::make_pair(c, boost::make_tuple(1, boost::get<2>(t))));
      }
      else
      {
         if ((x = ++boost::get<0>(it->second)) > max)
            max = x;

         boost::get<1>(it->second) = std::min(boost::get<1>(it->second),
                                              boost::get<2>(t));
      }
#ifdef DEBUG
      printf("***%c %lf %d\n", c, boost::get<2>(t), boost::get<1>(t));
#endif
   }

   char res = 0;
   d = DBL_MAX;
   BOOST_FOREACH(ResultsMapType::value_type &t, results)
   {
      if (boost::get<0>(t.second) == max)
         if (boost::get<1>(t.second) < d)
            d = boost::get<1>(t.second), res = t.first;
   }

   if (dist)
      *dist = d;
   
   return res;
}

void CharacterRecognizer::_loadFromFile( const std::string &filename )
{
   FILE *f = fopen(filename.c_str(), "r");
   int fonts_count, letters_count;
   fscanf(f, "%d %d\n", &_count, &letters_count);
   _classes.resize(letters_count);
   for (int i = 0; i < letters_count; i++)
   {
      SymbolClass &cls = _classes[i];
      fscanf(f, "%c %d\n", &cls.sym, &fonts_count);
      _mapping[cls.sym] = i;
      cls.shapes.resize(fonts_count);
      for (int j = 0; j < fonts_count; j++)
      {
         SymbolFeatures &sf = cls.shapes[j];
         fscanf(f, "%d\n", &sf.inner_contours_count);
         sf.descriptors.resize(2 * _count);
         for (int k = 0; k < 2 * _count; k++)
            fscanf(f, "%lf ", &sf.descriptors[k]);

         sf.inner_descriptors.resize(sf.inner_contours_count);
         for (int k = 0; k < sf.inner_contours_count; k++)
         {
            sf.inner_descriptors[k].resize(2 * _count);
            for (int l = 0; l < 2 * _count; l++)
               fscanf(f, "%lf ", &sf.inner_descriptors[k][l]);
         }
      }
   }

   fclose(f);
   _loaded = true;
}

HWCharacterRecognizer::HWCharacterRecognizer( const CharacterRecognizer &cr ) : _cr(cr)
{
   try
   {
      static const double h1[50] = { 0.123471, -0.532663, 0.816299, 0.150603, 0.250281, -0.086622, -0.685160, 0.403724, -0.180170, 0.352136, 0.178052, -0.329355, 0.024526, 0.291127, -0.232821, -0.288390, -0.097084, 0.095513, 0.017069, -0.012406, 0.118738, -0.014742, -0.084174, 0.015270, -0.046469, 0.006290, 0.010342, 0.024279, -0.153679, -0.037027, -0.003853, -0.057054, 0.074272, -0.082213, -0.025523, -0.034123, 0.057815, 0.059585, -0.035572, -0.005372, 0.022432, -0.086374, -0.030104, 0.028816, 0.021361, 0.008987, -0.027204, -0.007925, 0.080036, -0.061390};
      static const double h2[50] = { 0.329488, -0.628618, 0.754041, 0.250317, 0.210583, 0.103253, -0.768021, 0.247941, -0.193547, 0.164930, 0.405708, -0.000922, -0.332574, 0.019423, 0.101013, -0.151833, -0.247953, -0.030290, 0.131307, 0.147653, -0.016093, 0.003756, -0.014730, -0.084372, -0.109651, 0.082317, -0.094938, -0.025842, 0.040648, -0.141230, -0.025217, -0.027064, 0.086937, 0.024751, -0.036261, 0.014217, -0.037215, -0.041393, 0.029675, 0.007880, -0.026589, -0.060367, -0.005611, -0.012482, 0.016965, -0.032700, 0.047613, 0.028163, 0.018523, -0.052046};
      static const double h3[50] = { 0.153616, -0.520915, 0.893425, 0.074465, -0.159944, 0.013533, -0.669837, 0.668623, 0.180260, -0.191321, 0.020104, 0.036894, -0.089716, -0.056575, -0.292058, -0.019825, 0.181073, 0.004355, 0.012780, -0.019704, -0.060169, 0.039894, -0.054711, 0.020227, -0.067255, -0.057335, -0.069616, -0.014189, 0.075783, -0.107740, -0.027149, -0.030786, 0.068095, 0.046384, -0.020368, -0.018049, -0.024302, -0.048648, -0.007905, -0.007001, 0.050506, 0.033508, -0.016904, -0.045047, 0.026471, -0.100715, 0.010610, 0.046458, 0.082730, 0.068921};
      static const double h4[50] = { -0.048756, -0.170164, 1.150447, 0.235171, -0.115313, -0.116031, -0.516748, 0.796561, -0.150528, -0.054306, 0.351949, 0.293397, -0.116911, -0.034443, -0.370651, 0.128275, -0.056806, -0.102839, 0.064504, 0.034272, 0.012507, 0.052533, 0.058037, -0.007201, -0.034909, -0.040678, -0.073798, 0.146182, -0.024047, -0.017292, 0.051988, -0.007217, -0.034620, -0.021895, -0.076294, 0.030163, 0.011080, -0.078111, 0.029940, -0.074145, 0.047683, 0.034197, 0.038445, 0.024795, 0.006640, -0.023666, -0.019741, 0.024406, 0.030485, -0.028927};
      static const double h5[50] = {0.006842, -0.576943, 1.107501, 0.326954, 0.387111, -0.141601, -0.831790, 0.318668, 0.121318, 0.037255, 0.045324, 0.328056, 0.140457, 0.204640, -0.235057, -0.268401, -0.148869, 0.058260, -0.139784, 0.041308, 0.004354, -0.087098, 0.062147, -0.040822, 0.072725, 0.122862, -0.069856, -0.106719, -0.047416, 0.007898, 0.028911, 0.040743, -0.053454, 0.086117, 0.035020, 0.012754, -0.030376, -0.034087, -0.086174, -0.046656, -0.023136, -0.029601, 0.052690, -0.032009, -0.001256, 0.034847, 0.049781, -0.000791, -0.015903, -0.059238};
      static const double h6[50] = {0.655106, -0.813927, 0.850288, 0.418611, -0.119161, 0.328506, -0.821089, -0.057452, 0.209800, 0.434758, 0.115700, -0.158903, -0.159788, -0.134241, 0.042028, 0.114476, -0.148738, 0.070969, 0.039415, 0.093517, 0.025244, -0.020434, -0.179413, -0.022793, -0.072286, 0.008551, 0.067425, -0.059212, -0.007627, -0.020700, -0.047430, 0.099739, 0.015189, -0.014317, -0.124736, -0.037858, -0.038400, -0.033956, 0.074732, -0.076049, -0.028779, 0.053742, -0.016988, -0.014587, 0.010385, -0.007358, -0.034690, -0.023562, -0.058870, -0.077226};
      static const double n1[50] = { -0.106689, -0.566481, 1.017662, 0.084494, 0.469268, -0.262324, -0.761545, 0.389802, -0.198483, -0.029192, 0.059217, 0.067753, 0.227381, -0.111712, -0.151604, -0.122662, 0.192525, -0.011605, -0.046734, -0.024307, -0.140675, 0.153494, -0.025919, -0.002770, 0.030203, -0.088053, 0.002911, -0.067213, 0.024495, -0.015910, 0.001466, 0.049681, 0.011506, 0.015083, -0.081131, 0.032168, 0.002296, 0.062517, -0.019806, -0.071004, 0.024349, -0.064334, 0.013997, 0.014300, 0.005526, 0.063551, -0.050658, -0.022425, 0.013868, 0.037098};
      static const double n2[50] = { 0.387529, -0.778874, 0.864079, 0.681406, 0.065942, 0.098922, -0.092023, -0.535560, 0.031852, -0.091905, -0.324071, 0.317203, 0.045845, 0.119753, 0.114844, 0.001110, 0.068700, -0.125919, 0.009641, -0.007710, -0.002859, 0.075519, -0.040150, -0.036059, 0.039990, -0.033037, -0.034979, -0.006319, -0.007496, 0.075337, 0.032887, -0.039019, 0.036885, -0.024938, 0.017754, -0.040850, -0.021469, 0.044486, 0.042843, -0.008951, 0.003554, -0.015309, -0.006028, -0.082675, -0.047478, -0.012504, -0.019404, 0.027868, 0.033549, 0.017799};
      static const double n3[50] = { 0.959838, -0.846665, 0.728750, 0.608926, -0.315986, 0.299057, -0.400362, -0.362623, -0.203623, 0.450301, 0.176800, 0.064082, 0.161725, -0.127851, -0.285828, 0.021819, -0.140969, -0.055924, 0.059193, 0.077501, 0.085277, -0.025328, -0.030745, -0.011317, -0.087850, 0.044507, -0.021405, -0.034261, -0.009299, -0.029270, -0.014305, -0.024154, 0.011695, 0.024931, 0.035682, -0.024526, -0.023231, -0.031057, -0.015158, 0.052952, 0.030050, 0.009525, -0.048107, -0.045342, -0.011460, 0.011642, 0.016949, 0.009145, 0.014759, 0.006717};
      static const double n4[50] = { 0.023667, -0.211146, 1.286203, 0.306831, -0.152352, -0.073923, -0.874650, 0.547533, -0.000921, -0.135753, 0.102485, 0.105901, -0.003421, -0.024840, 0.110007, -0.006017, -0.149182, -0.000991, -0.211672, 0.260376, -0.019258, -0.142117, 0.032217, -0.065505, 0.005581, -0.004201, -0.042192, -0.003885, -0.042056, -0.028679, -0.028199, 0.097381, 0.002529, -0.095147, 0.008846, -0.088769, 0.048372, 0.012890, 0.002697, 0.052536, -0.036691, 0.000679, 0.013142, 0.038372, 0.024972, -0.059817, -0.001330, -0.017066, -0.000484, 0.046345};
#define FILL_FEATURES(f) do { \
      features_##f.inner_contours_count = 0; \
      features_##f.init = features_##f.recognizable = true; \
      features_##f.descriptors.assign(f, f + 50); } while (0);

      FILL_FEATURES(h1);
      FILL_FEATURES(h2);
      FILL_FEATURES(h3);
      FILL_FEATURES(h4);
      FILL_FEATURES(h5);
      FILL_FEATURES(h6);
      FILL_FEATURES(n1);
      FILL_FEATURES(n2);
      FILL_FEATURES(n3);
      FILL_FEATURES(n4);
#undef FILL_FEATURES

      /*{
       _readFile("h1.png", features_h1);
       _readFile("h2.png", features_h2);
       _readFile("h3.png", features_h3);
       _readFile("h4.png", features_h4);
       _readFile("n1.png", features_n1);
       _readFile("n2.png", features_n2);
       _readFile("n3.png", features_n3);
       _readFile("n4.png", features_n4);
       }*/
   }
   catch (Exception &e)
   {
      fprintf(stderr, "%s\n", e.what());
      return;
   }
}

void HWCharacterRecognizer::_readFile(const char *filename, SymbolFeatures &features)
{
   FileScanner scanner(filename);
   PngLoader loader(scanner);
   Image img;
      
   loader.loadImage(img);
   SegmentDeque segments;
      
   Segmentator::segmentate(img, segments, 3, 0);
   Segment &seg = **segments.begin();
   seg.initFeatures(25);
   features = seg.getFeatures();
}

int HWCharacterRecognizer::recognize (Segment &seg)
{
   seg.initFeatures(25);
   SymbolFeatures &features = seg.getFeatures();

   Segment thinseg;
   thinseg.copy(seg);
   ThinFilter2 tf(thinseg);
   tf.apply();

#ifdef DEBUG
   printf(" (%d ic)", features.inner_contours_count);
#endif
   
   if (isCircle(thinseg))
   {
#ifdef DEBUG
      printf(" circle ");
#endif
      return 'O';
   }


   double errh1 = CharacterRecognizer::_compareFeatures(features, features_h1);
   double errh2 = CharacterRecognizer::_compareFeatures(features, features_h2);
   double errh3 = CharacterRecognizer::_compareFeatures(features, features_h3);
   double errh4 = CharacterRecognizer::_compareFeatures(features, features_h4);
   double errh5 = CharacterRecognizer::_compareFeatures(features, features_h5);
   double errh6 = CharacterRecognizer::_compareFeatures(features, features_h6);
   double errn1 = CharacterRecognizer::_compareFeatures(features, features_n1);
   double errn2 = CharacterRecognizer::_compareFeatures(features, features_n2);
   double errn3 = CharacterRecognizer::_compareFeatures(features, features_n3);
   double errn4 = CharacterRecognizer::_compareFeatures(features, features_n4);

   double err_h[] = {errh1, errh2, errh3, errh4, errh5, errh6};
   double err_n[] = {errn1, errn2, errn3, errn4, 1e16, 1e16};

   int min_h = 0, min_n = 0;

   for (int i = 1; i < 6; i++)
   {
      if (err_h[i] < err_h[min_h])
         min_h = i;
      if (err_n[i] < err_n[min_n])
         min_n = i;
   }

   if (err_h[min_h] > 100)
      err_h[min_h] = 100;
   if (err_n[min_n] > 100)
      err_n[min_n] = 100;

#ifdef DEBUG
   printf(" h %.2lf", err_h[min_h]);
   printf(" n %.2lf\n", err_n[min_n]);
#endif

   if (err_h[min_h] < err_n[min_n])
   {
      if (err_h[min_h] < 3.2)
         return 'H';
   }
   else
   {
      if (err_n[min_n] < 3.45) //1.8
      {
         return 'N';
      }
   }

   static const std::string candidates =
      "ABCDFGHIKMNPRS"
      "aehiklnr" //u
      "1236";
   
   double err;
   char c = _cr.recognize(seg, candidates, &err);

#ifdef DEBUG
   if (c != 0)
      printf(" [%c] %.2lf \n", c, err);
#endif

   bool line = (c == 'l' || c == 'i' || c == '1');
   bool tricky = (c == 'r');
   bool hard = (c == 'R' || c == 'S');

   if (c == 'F' && err < 3.4)
      return c;

   if (line && err < 1.9) //0.5
      return c;

   if (tricky && err < 2.8) //1.0
      return c;

   if (!line && !tricky && err < 3.3) //1.8
      return c;

   if (hard && err < 4.5) //2.5
      return c;

#ifdef DEBUG
   puts("HWCR returned nothing");
   fflush(stdout);
#endif
   return -1;
}
