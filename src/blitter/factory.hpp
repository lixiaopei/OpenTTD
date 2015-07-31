/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file factory.hpp Factory to 'query' all available blitters. */

#ifndef BLITTER_FACTORY_HPP
#define BLITTER_FACTORY_HPP

#include "base.hpp"
#include "../debug.h"
#include "../string_func.h"
#include "../core/string_compare_type.hpp"
#include <map>

#if defined(WITH_COCOA)
bool QZ_CanDisplay8bpp();
#endif /* defined(WITH_COCOA) */

/**
 * The base factory, keeping track of all blitters.
 */
class BlitterFactory {
private:
	const char *name;        ///< The name of the blitter factory.
	const char *description; ///< The description of the blitter.
	
	/*用const char* 也可以作为map的key，作为比较方式，用StringCompare的struct来实现。这里的类型应该是BlitterFactory的map,所以命名是BlitterFactories比较好一些*/
	typedef std::map<const char *, BlitterFactory *, StringCompare> Blitters; ///< Map of blitter factories.

	/**
	 * Get the map with currently known blitters.
	 * @return The known blitters.
	 */
	//返回的是一个Blitters &,一个引用,厉害
	static Blitters &GetBlitters()
	{
		//这个声明好恐怖啊,直接声明一个map,然后它的地址是一个new出来的东西,既然new出来的,到最后也要删除了
		static Blitters &s_blitters = *new Blitters();
		return s_blitters;
	}

	/**
	 * Get the currently active blitter.
	 * @return The currently active blitter.
	 */
	static Blitter **GetActiveBlitter()
	{
		//如果没有呢,则创建一个,由于是把**的方式传递出去,所以外边可以反向赋值.
		//所以这里与其说是GetActiveBlitter,也可以修改未 CreateOrGetActiveBlitter()
		static Blitter *s_blitter = NULL;
		return &s_blitter;
	}

protected:
	/**
	 * Construct the blitter, and register it.
	 * @param name        The name of the blitter.
	 * @param description A longer description for the blitter.
	 * @param usable      Whether the blitter is usable (on the current computer). For example for disabling SSE blitters when the CPU can't handle them.
	 * @pre name != NULL.
	 * @pre description != NULL.
	 * @pre There is no blitter registered with this name.
	 */
	BlitterFactory(const char *name, const char *description, bool usable = true) :
			name(stredup(name)), description(stredup(description))
	{   //stredup,相当于malloc&copystr
		/*strdup（）函数是c语言中常用的一种字符串拷贝库函数，一般和free（）函数成对出现。
		*/
	
		if (usable) {
			/*
			 * Only add when the blitter is usable. Do not bail out or
			 * do more special things since the blitters are always
			 * instantiated upon start anyhow and freed upon shutdown.
			 */
			/*std::map的insert函数,返回结果有两个,iterator和bool,其中iterator应该是当前插入的对象的访问迭代器.
			  bool,表示这一次插入是否成功.
			*/
			std::pair<Blitters::iterator, bool> P = GetBlitters().insert(Blitters::value_type(this->name, this));
			assert(P.second);
		} else {
			DEBUG(driver, 1, "Not registering blitter %s as it is not usable", name);
		}
	}

public:
	virtual ~BlitterFactory()
	{		
		GetBlitters().erase(this->name);
		
		//果然做了delete,肩getBlitters()
		if (GetBlitters().empty()) delete &GetBlitters();

		//stredup得来的字符串,需要free
		free(this->name);
		free(this->description);
	}

	/**
	 * Find the requested blitter and return his class.
	 * @param name the blitter to select.
	 * @post Sets the blitter so GetCurrentBlitter() returns it too.
	 */
	static Blitter *SelectBlitter(const char *name)
	{
		BlitterFactory *b = GetBlitterFactory(name);
		if (b == NULL) return NULL;

		//用工厂生成一个Blitter
		Blitter *newb = b->CreateInstance();
		delete *GetActiveBlitter();
		//得到的是双重地址,可以复制的.
		*GetActiveBlitter() = newb;

		DEBUG(driver, 1, "Successfully %s blitter '%s'", StrEmpty(name) ? "probed" : "loaded", newb->GetName());
		return newb;
	}

	/**
	 * Get the blitter factory with the given name.
	 * @param name the blitter factory to select.
	 * @return The blitter factory, or NULL when there isn't one with the wanted name.
	 */
	static BlitterFactory *GetBlitterFactory(const char *name)
	{
#if defined(DEDICATED)
		const char *default_blitter = "null";
#else
		const char *default_blitter = "8bpp-optimized";

#if defined(WITH_COCOA)
		/* Some people reported lack of fullscreen support in 8 bpp mode.
		 * While we prefer 8 bpp since it's faster, we will still have to test for support. */
		if (!QZ_CanDisplay8bpp()) {
			/* The main display can't go to 8 bpp fullscreen mode.
			 * We will have to switch to 32 bpp by default. */
			default_blitter = "32bpp-anim";
		}
#endif /* defined(WITH_COCOA) */
#endif /* defined(DEDICATED) */
		if (GetBlitters().size() == 0) return NULL;
		const char *bname = (StrEmpty(name)) ? default_blitter : name;

		Blitters::iterator it = GetBlitters().begin();
		for (; it != GetBlitters().end(); it++) {
			BlitterFactory *b = (*it).second;
			if (strcasecmp(bname, b->name) == 0) {
				return b;
			}
		}
		return NULL;
	}

	/**
	 * Get the current active blitter (always set by calling SelectBlitter).
	 */
	static Blitter *GetCurrentBlitter()
	{
		return *GetActiveBlitter();
	}

	/**
	 * Fill a buffer with information about the blitters.
	 * @param p The buffer to fill.
	 * @param last The last element of the buffer.
	 * @return p The location till where we filled the buffer.
	 */
	static char *GetBlittersInfo(char *p, const char *last)
	{
		p += seprintf(p, last, "List of blitters:\n");
		Blitters::iterator it = GetBlitters().begin();
		for (; it != GetBlitters().end(); it++) {
			BlitterFactory *b = (*it).second;
			p += seprintf(p, last, "%18s: %s\n", b->name, b->GetDescription());
		}
		p += seprintf(p, last, "\n");

		return p;
	}

	/**
	 * Get the long, human readable, name for the Blitter-class.
	 */
	const char *GetName() const
	{
		return this->name;
	}

	/**
	 * Get a nice description of the blitter-class.
	 */
	const char *GetDescription() const
	{
		return this->description;
	}

	/**
	 * Create an instance of this Blitter-class.
	 */
	virtual Blitter *CreateInstance() = 0;
};

extern char *_ini_blitter;
extern bool _blitter_autodetected;

#endif /* BLITTER_FACTORY_HPP */
