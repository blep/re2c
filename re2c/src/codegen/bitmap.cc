#include <string.h> // memset

#include "src/codegen/bitmap.h"
#include "src/codegen/go.h"
#include "src/codegen/indent.h"
#include "src/globals.h"

namespace re2c
{

BitMap *BitMap::first = NULL;

BitMap::BitMap(const Go *g, const State *x)
	: go(g)
	, on(x)
	, next(first)
	, i(0)
	, m(0)
{
	first = this;
}

BitMap::~BitMap()
{
	delete next;
}

const BitMap *BitMap::find(const Go *g, const State *x)
{
	for (const BitMap *b = first; b; b = b->next)
	{
		if (matches(b->go->span, b->go->nSpans, b->on, g->span, g->nSpans, x))
		{
			return b;
		}
	}

	return new BitMap(g, x);
}

const BitMap *BitMap::find(const State *x)
{
	for (const BitMap *b = first; b; b = b->next)
	{
		if (b->on == x)
		{
			return b;
		}
	}

	return NULL;
}

static void doGen(const Go *g, const State *s, uint32_t *bm, uint32_t f, uint32_t m)
{
	Span *b = g->span, *e = &b[g->nSpans];
	uint32_t lb = 0;

	for (; b < e; ++b)
	{
		if (b->to == s)
		{
			for (; lb < b->ub && lb < 256; ++lb)
			{
				bm[lb-f] |= m;
			}
		}

		lb = b->ub;
	}
}

void BitMap::gen(OutputFile & o, uint32_t ind, uint32_t lb, uint32_t ub)
{
	if (first && bUsedYYBitmap)
	{
		o << indent(ind) << "static const unsigned char " << mapCodeName["yybm"] << "[] = {";

		uint32_t c = 1, n = ub - lb;
		const BitMap *cb = first;

		while((cb = cb->next) != NULL) {
			++c;
		}
		BitMap *b = first;

		uint32_t *bm = new uint32_t[n];
		
		for (uint32_t i = 0, t = 1; b; i += n, t += 8)
		{
			memset(bm, 0, n * sizeof(uint32_t));

			for (uint32_t m = 0x80; b && m; m >>= 1)
			{
				b->i = i;
				b->m = m;
				doGen(b->go, b->on, bm, lb, m);
				b = const_cast<BitMap*>(b->next);
			}

			if (c > 8)
			{
				o << "\n" << indent(ind+1) << "/* table " << t << " .. " << std::min(c, t+7) << ": " << i << " */";
			}

			for (uint32_t j = 0; j < n; ++j)
			{
				if (j % 8 == 0)
				{
					o << "\n" << indent(ind+1);
				}

				if (yybmHexTable)
				{
					o.write_hex (bm[j]);
				}
				else
				{
					o.write_uint32_t_width (bm[j], 3);
				}
				o  << ", ";
			}
		}

		o << "\n" << indent(ind) << "};\n";
		
		delete[] bm;
	}
}

} // end namespace re2c
