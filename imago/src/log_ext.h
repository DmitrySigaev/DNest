#pragma once
#ifndef _log_ext_h
#define _log_ext_h

#include <stdio.h>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include "segment.h"
#include "skeleton.h"
#include "segments_graph.h"
#include "image_utils.h"
#include "image_draw_utils.h"
#include "recognition_settings.h"
#include "current_session.h"
#include "virtual_fs.h"

#define logEnterFunction imago::log_ext_service::LogEnterFunction _entry(__FUNCTION__, imago::getLogExt()); _entry._logEnterFunction


namespace imago
{
	class log_ext;

	log_ext& getLogExt();

	struct FunctionRecord
	{
		std::string name;
		std::string anchor;
		size_t memory;
		size_t time;
		FunctionRecord(const std::string& n);
		std::string getPlatformSpecificInfo();
	};

	class log_ext
	{
	public:
		log_ext(const std::string folder);
		virtual ~log_ext();

		void SetVirtualFS(VirtualFS& vfs)
		{
			pVFS = &vfs;
			UseVirtualFS = true;
		}

		bool loggingEnabled() const;

		template <class t> void append(const std::string& name, const t& value)
		{
			if(!loggingEnabled()) return;

			std::ostringstream out;
			out << value;

			dump(getStringPrefix() + "<i>" + filterHtml(name) + "</i>: <u>" + filterHtml(out.str()) + "</u>");
		}

		template <class t> void appendVector(const std::string& name, const std::vector<t>& values)
		{
			if(!loggingEnabled()) return;

			dump(getStringPrefix() + constructTable(name, std::vector<size_t>(), values));
		}		

		template <class t1, class t2> void appendMap(const std::string& name, const std::map<t1,t2>& value)
		{
			if(!loggingEnabled()) return;

			std::vector<t1> row1;
			std::vector<t2> row2;

			for (typename std::map<t1,t2>::const_iterator it = value.begin(); it != value.end(); it++)
			{
				row1.push_back(it->first);
				row2.push_back(it->second);
			}			

			dump(getStringPrefix() + constructTable(name, row1, row2));
		}
		
		void appendText(const std::string& text);
		void appendImage(const std::string& caption, const Image& img);   
		void appendGraph(const std::string& name, const segments_graph::SegmentsGraph& g);   
		void appendSkeleton(const std::string& name, const Skeleton::SkeletonGraph& g);   
		void appendSegment(const std::string& name, const Segment& seg);
		void appendPoints(const std::string& name, const Points2i& pts);
		void appendSegmentWithYLine(const std::string& name, const Segment& seg, int line_y);

		void enterFunction(const std::string& name);
		void leaveFunction();

	private:
		FILE* FileOutput;
		bool UseVirtualFS;
		VirtualFS* pVFS;
		std::string Folder;
		size_t ImgIdent, CallIdent;
		std::vector<FunctionRecord> Stack;

		std::string generateAnchor(const std::string& name);
		std::string generateImageName(std::string* html_name = NULL);
		std::string getStringPrefix(bool paragraph = false) const;
		std::string filterHtml(const std::string source) const;
		void dump(const std::string& data);
		void dumpImage(const std::string& filename, const Image& data);

		template <class t1, class t2> std::string constructTable(const std::string& caption, 
			                                                     const std::vector<t1>& row1, 
																 const std::vector<t2>& row2)
		{
			std::string table = "<table style=\"display:inline;\"><thead>";
			table +=            "<tr><th colspan=\"2\">" + filterHtml(caption) + 
								"</th></tr></thead><tbody>";
		
			if (!row1.empty())
			{
				table += "<tr>";
				for (size_t pos = 0; pos < row1.size(); pos++)
				{
					std::ostringstream visual;
					visual << row1[pos];
					table += "<td>" + filterHtml(visual.str()) + "</td>";
				}
				table += "</tr>";
			}

			if (!row2.empty())
			{
				table += "<tr>";
				for (size_t pos = 0; pos < row2.size(); pos++)
				{
					std::ostringstream visual;
					visual << row2[pos];
					table += "<td>" + filterHtml(visual.str()) + "</td>";
				}
				table += "</tr>";
			}

			table += "</tbody></table>";

			return table;
		}		
	}; /// end class log_ext
   

	namespace log_ext_service
	{
		class LogEnterFunction
		{
		public:
			LogEnterFunction(const std::string& name, log_ext& log) : Log(log)
			{
				Log.enterFunction(name);
			}
			~LogEnterFunction()
			{
				Log.leaveFunction();
			}
			void _logEnterFunction() // fake stub method for macros calling decoration
			{
			}
		private:
			log_ext& Log;
		};
	}
	
} // end namespace

#endif // _log_ext_h
