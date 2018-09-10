#include "complex_contour.h"
#include "settings.h"
#include "approximator.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/graph_traits.hpp>
#include "algebra.h"
#include "log_ext.h"
#include <opencv2/opencv.hpp>

using namespace imago;

ComplexContour::ComplexContour(void)
{
}


ComplexContour::~ComplexContour(void)
{
}

ComplexNumber& ComplexContour::getContour(int shift)
{
			  
	return *(_contours[shift % _contours.size()]);
}

double ComplexContour::DiffR2(ComplexContour lc)
{
	double max1 = 0;
	double max2 = 0;
	double sum = 0;
	for(size_t i = 0; i< _contours.size();i++)
	{
		double v1 = _contours[i].getRadius();
		double v2 = lc.getContour(i).getRadius();
		if(v1 > max1) max1 = v1;
		if(v2 > max2) max2 = v2;
		double v = v1 - v2;
		sum += v *v;
	}

	double max = std::max(max1, max2);
	return 1. - sum / _contours.size() / max / max;
}


double ComplexContour::Norm() const
{
	double result = 0;
	for(size_t i = 0;i < _contours.size();i++)
	{
		result += _contours[i].getRadius2();
	}
	return sqrt(result);
}

ComplexNumber ComplexContour::Dot(ComplexContour c, int shift)
{
	ComplexNumber cn = ComplexNumber(0, 0);
	for(size_t i=0;i<_contours.size();i++)
		cn = cn + ComplexNumber::Dot(_contours[i], c.getContour(shift + i));

	return cn;
}

std::vector<ComplexNumber> ComplexContour::InterCorrelation(ComplexContour c)
{
	int count = _contours.size();
	std::vector<ComplexNumber> retVal;
	for(int i=0;i< count;i++)
		retVal.push_back(Dot(c, i));
	return retVal;
}

std::vector<ComplexNumber> ComplexContour::AutoCorrelation(bool normalize)
{
	int count = _contours.size()/2;// > 0 ? _contours.size() : 1 ;
	double maxNorm = 0;
	std::vector<ComplexNumber> acf;
	for(int i=0; i<count; i++)
	{
		ComplexNumber cn = Dot(*this, i);
		
		acf.push_back(cn);

		double normaSq = acf[i].getRadius2();
		if(normaSq > maxNorm)
			maxNorm = normaSq;

	}

	if(normalize && maxNorm > 0)
	{
		double maxNormaSq = sqrt(maxNorm);
		for(size_t i=0;i< acf.size();i++)
			acf[i] /= maxNormaSq;
	}

	return acf;
}

ComplexNumber ComplexContour::FindMaxNorm()
{
	double max = 0.;
	ComplexNumber res(0, 0);
	for(size_t i =0;i<_contours.size();i++)
		if(_contours[i].getRadius() > max)
		{
			max = _contours[i].getRadius();
			res = _contours[i];
		}
				  
return res;
}

void ComplexContour::Scale(double scale)
{
	for(size_t i=0; i< _contours.size();i++)
		_contours[i] *= scale;
}

void ComplexContour::Normalize()
{
	double max = FindMaxNorm().getRadius();
	if(max > 0)
		Scale(1.0/max);
}

void ComplexContour::NormalizeByPerimeter()
{
	double perimeter = 0;
	for(size_t i = 0; i < _contours.size(); i++)
		perimeter += _contours[i].getRadius();
	if(perimeter > 0)
		Scale(1.0 / perimeter);
}

double ComplexContour::getNorm()
{
	double result = 0.;
	for(size_t i=0;i<_contours.size();i++)
		result += _contours[i].getRadius2();
	return std::sqrt(result);
}

double ComplexContour::Distance(ComplexContour c)
{
	double n1 = this->Norm();
	double n2 = c.Norm();
	return n1 * n1 + n2 * n2 - 2 * (Dot(c).getReal());
}

void ComplexContour::EqualizeUp(int n)
{
	ComplexNumber currPoint = _contours[0];
	int count = _contours.size();

	std::vector<ComplexNumber> newCont;

	
	int times = (n - count) / count;
	int span = (n - count) % count;

	for(int i=0; i < count;i ++)
	{
		int slice = i < span ? times + 1 : times;

		if(slice > 0)
		{
			double k = 1.0 / (1.0 + slice);

			ComplexNumber c = _contours[i];
			c *= k;
			for(int k = 0 ; k <= slice; k++)
				newCont.push_back(c);
		}
		else
			newCont.push_back(_contours[i]);
	}

	_contours = newCont;
}

void ComplexContour::EqualizeDown(int n)
{
	int count = _contours.size();
	ComplexNumber currPoint = _contours[0];

	std::vector<ComplexNumber> newCont;

	for(int i = 0; i< n; i++)
		newCont.push_back(ComplexNumber());

	for(int i = 0; i < count; i++)
	{
		newCont[i * n / count] += _contours[i];
	}

	_contours = newCont;
}

void ComplexContour::Equalize(int n)
{
	if(n > (int)_contours.size())
		EqualizeUp(n);
	else
		if(n < (int)_contours.size())
			EqualizeDown(n);
}


void cvRetrieveContour(Image& img, Points2d &lines, int eps)
{
	int w = img.getWidth(), h = img.getHeight();
   cv::Mat mat = cv::Mat::zeros(cv::Size(w + 2, h + 2), CV_8U);
   
   for (int i = 0; i < w; i++)
      for (int j = 0; j < h; j++)
         mat.at<unsigned char>(j+1, i+1) = 255 - img.getByte(i, j);
   
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Point> newcont;
    std::vector<cv::Vec4i> hierarchy;
	cv::findContours(mat, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_KCOS);
	if (!contours.empty())
	{
		cv::Mat m(contours[0]);
		cv::approxPolyDP(contours[0], newcont, eps, false);
	
		for(size_t i = 0;i < newcont.size(); i++)
		{
			lines.push_back(Vec2d(newcont[i].x, newcont[i].y));
		}
	}
}

ComplexContour ComplexContour::RetrieveContour(const Settings& vars, Image& seg)
{
	logEnterFunction();
	std::vector<ComplexNumber> contours;
	double lnThickness = vars.estimation.LineThickness;

	Points2d lines;
	
	double eps = (lnThickness / 2.0 > 2.0) ? (lnThickness / 2.0) : 2.0;

	cvRetrieveContour(seg, lines, imago::round(eps)); 

	Skeleton graph;
	Vec2d lastPoint;
	
	imago::Skeleton::Vertex vStart = graph.addVertex(lines[0]);
	imago::Skeleton::Vertex vStart1 = vStart;

	// add lines to graph
	if (!lines.empty())
	{
	   for (size_t i = 1; i < lines.size() ; i++)
	   {	  
		   imago::Skeleton::Vertex vEnd = graph.addVertex(lines[i]);
		   //graph.addBond(p1, p2, SINGLE, true);
		   graph.addBond(vStart, vEnd);
		   vStart = vEnd; //graph.addVertex(lines[i]);
		  //lastPoint = p2;
	   }

	   //graph.addBond(lastPoint, lines[0], SINGLE, true);
	   graph.addBond(vStart, vStart1);
	}
	else
	{
		throw LogicException("No contours");
	}

	//graph._joinVertices(0.01);
	Skeleton::SkeletonGraph _g = graph.getGraph();
	getLogExt().appendSkeleton(vars, "after join vertices", _g);
	Skeleton::Vertex vert1 = *(_g.m_vertices.begin());
	Skeleton::Vertex vert2;
	Skeleton::Vertex vertIt = vert1;

	std::deque<Skeleton::Vertex> neighbours;
	std::deque<Skeleton::Vertex>::iterator vit, minIt;
	boost::graph_traits<Skeleton::SkeletonGraph>::adjacency_iterator b, e;
  
	vertIt = vStart1;
	boost::tie(b, e) = boost::adjacent_vertices(vStart1, _g);
	neighbours.assign(b, e);
	//TODO: FIND appropriate vert2 for a clockwise traversal 

	vert2 = *(neighbours.begin());

	do
	{
		double min_angle = 2*PI;
		
			//add contour
			Vec2d bpos = boost::get(boost::vertex_pos, _g, vert1);
			Vec2d epos = boost::get(boost::vertex_pos, _g, vert2);

			contours.push_back(ComplexNumber(bpos.x, bpos.y));
			contours.push_back(ComplexNumber(epos.x, epos.y));

			//find adjacent vertices of vert2
			std::deque<Skeleton::Vertex> neighbours2;
			std::deque<Skeleton::Vertex>::iterator vit2;
			boost::graph_traits<Skeleton::SkeletonGraph>::adjacency_iterator b2, e2;
			boost::tie(b2, e2) = boost::adjacent_vertices(vert2, _g);
			neighbours2.assign(b2, e2);

			minIt = neighbours2.begin();

			//find adjacent vertex to vert2  with minimum angle with (vert1, vert2) edge
			for(vit2 = neighbours2.begin(); vit2 != neighbours2.end(); vit2++)
			{
				Skeleton::Vertex v = *(vit2);

				if(v == vert1)
					continue;

				Vec2d vpos = boost::get(boost::vertex_pos, _g, v);

				Vec2d e1, e2, e;
				e1.diff(bpos, epos);
				e2.diff(vpos, epos);

				double angle1 = atan2(e1.y, e1.x);
				if(angle1 < 0 )
					angle1 = 2 * PI + angle1;
				double angle2 = atan2(e2.y, e2.x);
				if(angle2 < 0)
					angle2 = 2 * PI + angle2;

				double angle = angle1 - angle2;

				angle = angle < 0 ? 2*PI + angle : angle;

				if( min_angle > angle)
				{
					min_angle = angle;
					minIt = vit2;
				}
			}

			vert1 = vert2;
			vert2 = *(minIt);

	}while(vert1 != vertIt);

	std::string directions;
	double pi_8 = imago::PI / 8.0;

	std::vector<ComplexNumber> diffCont;
	//int i;
	for(size_t i=1;i < contours.size();i+=2)
	{
		ComplexNumber c = contours[i] - contours[i-1];
		
		if(getLogExt().loggingEnabled())
		{
			double angle = c.getAngle();
			if(angle < 0)
				angle  += 2 * PI;
			directions += " ";
			if(angle < pi_8 || angle >= 15.0 * pi_8)
				directions += "E";
			else
				if(angle >= pi_8 && angle < 3.0 * pi_8)
					directions += "NE";
				else
					if(angle >= 3.0 * pi_8 && angle < pi_8 * 5.0)
						directions += "N";
					else
						if(angle >= pi_8 * 5.0 && angle < pi_8 * 7.0)
							directions += "NW";
						else
							if(angle >= pi_8 * 7.0 && angle < pi_8 * 9.0)
								directions += "W";
		
			if(angle >= 9.0 * pi_8 && angle < 11.0 * pi_8)
					directions += "SW";
				else
					if(angle >= 11.0 * pi_8 && angle < pi_8 * 13.0)
						directions += "S";
					else
						if(angle >= pi_8 * 13.0 && angle < pi_8 * 15.0)
							directions += "SE";
			getLogExt().appendText(directions);
		}
		diffCont.push_back(c);
	}

	

	return ComplexContour(diffCont);
}