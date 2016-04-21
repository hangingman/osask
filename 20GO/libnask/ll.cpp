/* "LL.c":Lilte-Linker */
/* Copyright(C) 2002 H.Kawai */

/* 方針：遅くてもいいから、とりあえず動くこと！ */
/* アルゴリズムに凝るのは、動くようになってから */

#include <cstdlib>
#include "ll.hpp"

#if (DEBUG)
	#include "go.hpp"
	#include <cstdio>
#endif

UCHAR *LL_skipcode(UCHAR *p)
{
	LOG_DEBUG("in: %s \n", dump_ptr("p", p).c_str());
	UCHAR c, len;
retry:
	c = *p++;
	if (0x30 <= c && c <= 0x37) {
		p += c - 0x30; /* 0〜7バイト出力 */
		goto fin;
	}
	if (0x78 <= c && c <= 0x7f)
		goto fin; /* virtual-byte展開 */
	if (0x70 <= c && c <= 0x77)
		goto retry; /* virtual-byte定義 */
	if (0xe0 <= c && c <= 0xef)
		goto fin; /* パラメータなし1バイトリマーク */
	if (0xf0 <= c && c <= 0xf7) {
		p += c - (0xf0 - 1); /* パラメータ付きリマーク(1〜8) */
		goto fin;
	}
	if (c == 0x0e) {
		p = LL_skip_expr(p); /* プログラムカウンタラベル定義 */
		goto fin;
	}
	if (c == 0x2d) {
		p = LL_skip_expr(p); /* equラベル定義 */
		p = LL_skip_expr(p);
		goto fin;
	}
	if (0x0c <= c && c <= 0x0d) { /* equラベル展開 */
		p += 4;
		goto fin;
	}
	if (c == 0x0f) {
		p += p[1] - (0x08 - 3); /* GLOBAL用変換 */
		goto fin;
		/* 0x0f 3 0x08 xx */
	}
	if (c == 0x2c) { /* externラベル定義 */
		p = LL_skip_expr(p);
		goto fin;
	}
	if (0x2e <= c && c <= 0x2f) { /* リロケーション情報 */
		p += 3;
		goto fin;
	}
	if (0x38 <= c && c <= 0x3f) {
		p = LL_skip_expr(p + 1); /* 1〜8バイトを出力 */
		goto fin;
	}
	if (c == 0x58) {
		p = LL_skip_expr(p); /* ORG */
		goto fin;
	}
	if (c == 0x5a) { /* ORG処理後 */
		p += 4;
		goto fin;
	}
	if (c == 0x68) {
		p++; /* endian */
		goto fin;
	}
	if (0x90 <= c && c <= 0x91) {
		/* microcode90 */
		/* microcode91 */
		len = ((p[2] /* z-bbb-d-aaa */ >> 7) & 0x01) + 2;
		p = LL_skip_expr(p + 3 /* code, decision, zbbbdaaa */);
		if (c == 0x91)
			len *= 2;
		do {
			p = LL_skip_mc30(p, NULL, 1);
		} while (--len);
		goto fin;
	}
	if (0x94 <= c && c <= 0x95) {
		/* microcode94 */
		/* microcode95 */
		p = LL_skip_expr(p + 3);
		len = *p++; /* len */
		goto skipmc94_skip;
		do {
			p = LL_skip_expr(p);
skipmc94_skip:
			p = LL_skip_mc30(p, NULL, 0);
			p = LL_skip_mc30(p, NULL, 1);
			if (c == 0x95)
				p = LL_skip_mc30(p, NULL, 1);
		} while (--len);
		goto fin;
	}
	if (c == 0x59) {
		LOG_DEBUG("TIMES microcode \n");
		p = LL_skip_expr(p + 4); /* TIMES microcode (prefix) */
		p = LL_skip_expr(p);
		goto fin;
	}

	#if (DEBUG)
		fprintf(stderr, "LL_skipcode error:%02X\n", c);
		//GOL_sysabort(GO_TERM_BUGTRAP);
	#endif

fin:
	return p;
}

UCHAR *LL_skip_expr(UCHAR *expr)
{
	UCHAR c;
	c = *expr++;
	if (c <= 6) {
		/* 定数 */
		c >>= 1;
		expr += c + 1;
		goto fin;
	}
	if (c == 0x07)
		goto fin;
	if (c <= 0x0b) {
		/* ラベル引用 */
		expr += c - 7; /* c - 8 + 1 */
		goto fin;
	}
	#if (DEBUG)
		if (c < 0x10)
			goto dberr;
	#endif
	if (c < 0x20) {
		expr = LL_skip_expr(expr);
		if (c <= 0x12)
			goto fin; /* 単項演算子 */
		expr = LL_skip_expr(expr);
		goto fin; /* 二項演算子 */
	}

	#if (DEBUG)
		if (c < 0x80)
			goto dberr;
	#endif
	if (c == 0x80) {
		/* subsect size sum */
		expr = LL_skip_expr(expr);
		expr = LL_skip_expr(expr);
		goto fin;
	}
	#if (DEBUG)
dberr:
		fprintf(stderr, "LL_skip_expr:%02x\n", c);
		//GOL_sysabort(GO_TERM_BUGTRAP);
	#endif

fin:
	return expr;
}

UCHAR *LL_skip_mc30(UCHAR *s, UCHAR *bytes, char flag)
{
	static char mc98_width[] = { 1, 1, 2, 2, 4, 4, 1 };
	UCHAR c;

	c = *s++;
	#if (DEBUG)
		if (c < 0x30)
		goto dberr;
	#endif
	if (c < 0x38) {
		s += (c &= 0x07);
		goto fin;
	}
	if (c < 0x3c) {
		c -= 0x37;
		s = LL_skip_expr(s + 1 /* レンジを読み飛ばす */);
		goto fin;
	}
	#if (DEBUG)
		if (c < 0x98)
			goto dberr;
	#endif
	if (c < 0x9f) {
		c = mc98_width[c & 0x07];
		if (flag)
			s = LL_skip_expr(s);
		goto fin;
	}
	#if (DEBUG)
dberr:
		fprintf(stderr, "LL_skip_mc30:%02x\n", c);
		//GOL_sysabort(GO_TERM_BUGTRAP);
	#endif
fin:
	if (bytes)
		*bytes = c;
	return s;
}

void init_value(STR_VALUE* value)
{
	LOG_DEBUG("in \n");
	int i;
	value->min = value->delta = value->scale[0] = value->scale[1] = 0;
	value->flags = 0;
	value->label[0] = value->label[1] = -1;
	for (i = 0; i < MAXSIGMAS; i++) {
		value->sigma[i].scale = 0;
		value->sigma[i].subsect = 0;
		value->sigma[i].terms = 0;
	}
	LOG_DEBUG("initilized value: %s \n", value->to_string().c_str());
	return;
}

unsigned int get_id(int len, UCHAR **ps, int i)
/* len = 0〜3 */
{
	union UINT_UCHAR {
		unsigned int i;
		UCHAR c[4];
	} u;
	UCHAR *s = *ps;
	u.i = i;
	i = 0;
	do {
		u.c[i++] = *s++;
	} while (--len >= 0);
	*ps = s;
	return u.i;
}

void calc_value(std::unique_ptr<STR_VALUE>& value, UCHAR **pexpr)
{
	LOG_DEBUG("in \n");
	calc_value0(value, pexpr);
	if (value->sigma[0].scale)
		calcsigma(value);
	return;
}

void calc_value0(std::unique_ptr<STR_VALUE>& value, UCHAR **pexpr)
{
	UCHAR *expr = *pexpr, c, *t;
	int i, j, k;
	std::unique_ptr<STR_VALUE> tmp, tmp2;
	struct STR_SUBSECTION *subsect;
	c = *expr++;

	LOG_DEBUG("in, 0x%02x \n", c);
	init_value(value.get());
	if (c <= 6) {
		/* 定数 */
		i = 0;
		if (c & 0x01)
			i--;
		value->min = get_id(c >> 1, &expr, i);
		goto fin;
	}
	if (c == 0x07) {
		/* 定数1 */
		value->min = 1;
		goto fin;
	}
	if (c <= 0x0b) {
		/* ラベル引用 */
	     	std::shared_ptr<STR_LABEL> label(&label0[i = get_id(c - 8, &expr, 0)]);
		if ((label->value.flags & VFLG_ENABLE) == 0) {
			if (label->value.flags & VFLG_CALC) {
				value->flags |= VFLG_SLFREF;
				goto fin;
			}
			enable_label(label.get());
		}
		*value = label->value;
		goto fin;
	}
	#if (DEBUG)
		if (c < 0x10)
			goto dberr;
	#endif
	if (c < 0x20) {
		/* 演算子 */
		calc_value0(value, &expr);
		if (c == 0x10)
			goto fin; /* 単項演算子 + */
		if (c == 0x11) {
			/* 単項演算子 - */
			value->min = - (int) (value->min + value->delta);
	minus2:
			value->scale[0] *= -1;
 			value->scale[1] *= -1;
			for (i = 0; i < MAXSIGMAS; i++)
				value->sigma[i].scale *= -1;
			goto fin;
		}
		if (c == 0x12) {
			/* 単項演算子 ~ */
			/* ~a == - a - 1 */
			value->min = - (int) (value->min + value->delta) - 1;
			goto minus2;
		}

		calc_value0(tmp, &expr);
		value->flags |= tmp->flags;
		if (c == 0x13) {
			/* 二項演算子 + */
add2:
			for (i = 0; i < 2; i++) {
				if (value->scale[i] == 0)
					break;
			}
			for (j = 0; j < 2; j++) {
				if (tmp->scale[j] == 0)
					break;
				for (k = 0; k < i; k++) {
					if (value->label[k] == tmp->label[j]) {
						/* 同類項検出 */
						if ((value->scale[k] += tmp->scale[j]) == 0) {
							/* 項の消滅 */
							i--;
							value->label[k] = 0xffffffff;
							if (k)
								goto next_product;
							value->scale[0] = value->scale[1];
							value->label[0] = value->label[1];
							value->scale[1] = 0;
							value->label[1] = 0xffffffff;
						}
						goto next_product;
					}
				}
				/* 同類項が無かったので、項の追加 */
				if (i >= 2) {
					value->flags |= VFLG_ERROR;
					goto fin;
				}
				value->scale[i] = tmp->scale[j];
				value->label[i] = tmp->label[j];
				i++;
next_product:
				;
			}
			for (j = 0; j < MAXSIGMAS; j++) {
				if (tmp->sigma[j].scale == 0)
					break;
				addsigma(value, tmp->sigma[j]);
			}
			value->min += tmp->min;
			value->delta += tmp->delta;
			goto fin;
		}
		if (c == 0x14) {
			/* 二項演算子 - */
			tmp->min = - (int) (tmp->min + tmp->delta);
			tmp->scale[0] *= -1;
 			tmp->scale[1] *= -1;
			for (i = 0; i < MAXSIGMAS; i++)
				tmp->sigma[i].scale *= -1;
			goto add2;
		}
		if (c == 0x15) { /* "*" */
			if (value->scale[0])
				goto no_exchange;
			if (tmp->scale[0])
				goto exchange;
		}
		if (c == 0x15 || 0x1d <= c && c <= 0x1f) { /* "*", "&", "|", "^" */
			if (value->delta)
				goto no_exchange;
			if (tmp->delta)
				goto exchange;
			if (value->sigma[0].scale)
				goto no_exchange;
			if (tmp->sigma[0].scale) {
		exchange:
				/* 項の交換 */
				/* 不定値を1項目に持ってくるために交換 */
				tmp->flags = value->flags;
				//tmp2 = *value;
				//*value = tmp;
				//tmp = tmp2;
				// FIXME: 要テスト
				std::swap(tmp, value);
			}
		}
	no_exchange:
		calcsigma(tmp);
		if (tmp->delta) {
			/* 不定値を使うために、値がまだ決定できない */
			value->flags |= VFLG_ERROR;
			goto fin;
		}
		if (c == 0x15) {
			/* 二項演算子 * (signed) */
			if (tmp->scale[0]) {
	error:
				/* 両方ともlabelを含んでいればエラー */
				value->flags |= VFLG_ERROR;
				goto fin;
			}
multiply2:
			if (tmp->min == 0) {
				init_value(value.get());
				goto fin;
			}
			value->scale[0] *= tmp->min;
			value->scale[1] *= tmp->min;
			for (i = 0; i < MAXSIGMAS; i++)
				value->sigma[i].scale *= tmp->min;
			i = (signed int) value->min * (signed int) tmp->min;
			j = (signed int) (value->min + value->delta) * (signed int) tmp->min;
minmax:
			value->min = i;
			value->delta = j - i;
			if (i > j) {
				value->min = j;
				value->delta = i - j;
			}
			goto fin;
		}
		/* これ以降はtmp側のlabelはキャンセルされる */
		if (c == 0x16) {
			/* 二項演算子 << */
			tmp->min = 1 << tmp->min;
			goto multiply2;
		}
		if (c == 0x17) {
			/* 二項演算子 / (unsigned) */
divu2:
			if (tmp->min == 0)
				goto error;
divu_ij:
			i = (unsigned int) value->min / (unsigned int) tmp->min;
			j = (unsigned int) (value->min + value->delta) / (unsigned int) tmp->min;
			for (k = 0; k < MAXSIGMAS; k++) {
				if (value->sigma[k].scale % tmp->min) {
					calcsigma(value);
					goto divu_ij;
				}
			}
divall:
			if (value->scale[0] % tmp->min)
				goto error;
			if (value->scale[1] % tmp->min)
				goto error;
			value->scale[0] /= tmp->min;
			value->scale[1] /= tmp->min;
			for (k = 0; k < MAXSIGMAS; k++)
				value->sigma[k].scale /= tmp->min;
			goto minmax;
		}
		if (c == 0x19) {
			/* 二項演算子 / (signed) */
divs2:
			if (tmp->min == 0)
				goto error;
divs_ij:
			i = (signed int) value->min / (signed int) tmp->min;
			j = (signed int) (value->min + value->delta) / (signed int) tmp->min;
			for (k = 0; k < MAXSIGMAS; k++) {
				if (value->sigma[k].scale % tmp->min) {
					calcsigma(value);
					goto divs_ij;
				}
			}
			goto divall;
		}
		if (c == 0x1b) {
			/* 二項演算子 >> (unsigned) */
			tmp->min = 1 << tmp->min;
			goto divu2;
		}
		if (c == 0x1c) {
			/* 二項演算子 >> (signed) */
			tmp->min = 1 << tmp->min;
			goto divs2;
		}
		value->scale[0] = value->scale[1] = 0; /* 自動キャンセル */
		calcsigma(value);
		if (c == 0x18) {
			/* 二項演算子 % (unsigned) */
			if (tmp->min == 0)
				goto error;
			value->min = (unsigned int) value->min % (unsigned int) tmp->min;
			if (value->delta) {
				value->min = 0;
				value->delta = tmp->min - 1;
			}
			goto fin;
		}
		if (c == 0x1a) {
			/* 二項演算子 % (signed) */
			if (tmp->min == 0)
				goto error;
			value->min = (signed int) value->min % (signed int) tmp->min;
			if (value->delta) {
				value->delta = tmp->min - 1;
				if (value->min >= 0) {
					value->min = 0;
				} else if ((signed int) (value->min + value->delta) <= 0) {
					value->min = 1 - tmp->min;
				} else {
					value->min = 1 - tmp->min;
					value->delta *= 2;
				}
			}
			goto fin;
		}
		if (c == 0x1d) {
			/* 二項演算子 & (unsigned) */
			if (tmp->min == -1)
				goto fin; /* 素通り */
			if (value->delta == 0) {
				value->min &= tmp->min;
				goto fin;
			}
			if (tmp->min & 0x80000000)
				goto error; /* 手が付けられない */
			/* 本当はもっと細かくできるが、面倒なのでやってない */
			value->min = 0;
			value->delta = tmp->min;
			goto fin;
		}
		if (c == 0x1e) {
			/* 二項演算子 | (unsigned) */
			if (tmp->min == 0)
				goto fin; /* 素通り */
			if (value->delta == 0) {
				value->min |= tmp->min;
				goto fin;
			}
			if (tmp->min & 0x80000000)
				goto error; /* 手が付けられない */
			value->delta += tmp->min;
			if ((value->delta & 0x80000000) == 0)
				goto fin;
			value->delta = 0;
			goto error;
		}
	//	if (c == 0x1f) {
			/* 二項演算子 ^ (unsigned) */
			if (tmp->min == 0)
				goto fin; /* 素通り */
			if (value->delta == 0) {
				value->min ^= tmp->min;
				goto fin;
			}
			if (tmp->min & 0x80000000)
				goto error; /* 手が付けられない */
			if ((signed int) value->min > (signed int) 0 && value->min >= tmp->min) {
				value->min -= tmp->min;
				value->delta += tmp->min;
				goto fin;
			}
			goto error; /* 手が付けられない */
	//	}
	}
	#if (DEBUG)
		if (c < 0x80)
			goto dberr;
	#endif
#if 0
	if (c == 0x80) {
		/* subsect size sum */
		/* sumは、deltaが2GBならエラーにしてしまう(面倒なので) */
		calc_value(&tmp, &expr);
		i = tmp->min;
		calc_value(&tmp, &expr);
		subsect = subsect0 + tmp->min;
		do {
			value->min += subsect->min;
			value->delta += subsect->delta;
			if (subsect->delta >= INVALID_DELTA)
				value->flags |= VFLG_ERROR;
		} while (--i);
		goto fin;
	}
#endif
	#if (DEBUG)
dberr:
		fprintf(stderr, "calc_value:%02x\n", c);
		//GOL_sysabort(GO_TERM_BUGTRAP);
	#endif

fin:
	*pexpr = expr;
	return;
}

void enable_label(STR_LABEL* label)
{
	std::unique_ptr<STR_VALUE> value;
	UCHAR *t;

//	if (label->value.flags & VFLG_CALC) {
//		label->value.flags |= VFLG_SLFREF;
//		return;
//	}
//	init_value(&value);
//	value.label[0] = label - label0; /* for EXTERN */
//	value.scale[0] = 1;
//	value.flags |= label->value.flags & VFLG_EXTERN;
	if ((t = label->define) != NULL) {
		label->value.flags |= VFLG_CALC;
		calc_value0(value, &t);
		if (value->flags & VFLG_SLFREF) {
			init_value(value.get());
			value->flags |= VFLG_SLFREF;
		}
	} else /* if ((label->value.flags & VFLG_EXTERN) == 0) */ {
		/* EXTERNの時はすぐにENABLEになるので、ここに来るはずない */
		init_value(value.get());
		value->flags |= VFLG_UNDEF;
	}
	label->value = value;
	label->value.flags |= VFLG_ENABLE;
	return;
}

static UCHAR *skip_expr(UCHAR *expr)
{
	return LL_skip_expr(expr);
}

template <class T>
void manually_increment(std::shared_ptr<T>& ptr)
{
     auto ptr_it = ptr.get();
     ptr_it++;
}

UCHAR *LL_define_VB(struct STR_LL_VB *virtualbyte, UCHAR *s)
{
	UCHAR c;
	virtualbyte->typlen = c = *s++;
	#if (DEBUG)
		if (c < 0x30)
			goto dberr0;
	#endif
	if (c < 0x38) {
		virtualbyte->expr = s;
		s += c & 0x07;
		goto fin;
	}
	if (c < 0x3c)
		virtualbyte->range = *s++;
	else {
		#if (DEBUG)
			if (c < 0x98)
				goto dberr0;
			if (c >= 0x9f)
				goto dberr0;
		#endif
		virtualbyte->typlen = table98typlen[c & 0x07];
		virtualbyte->range  = table98range [c & 0x07];
	}
	virtualbyte->expr = s;
	s = skip_expr(s);
fin:
	return s;

	#if (DEBUG)
dberr0:
		fprintf(stderr, "LL_define_VB:%02x\n", c);
		//GOL_sysabort(GO_TERM_BUGTRAP);
		return NULL;
	#endif
}

UCHAR *lccbug_LL_mc90_func(UCHAR *src0, struct STR_LL_VB *vbb, struct STR_LL_VB *vbc, char len, char c)
{
	do {
		if (c != 0) {
			src0 = skip_mc30(src0, NULL, 1);
			if (vbc)
				src0 = skip_mc30(src0, NULL, 1);
		} else {
			src0 = LL_define_VB(vbb, src0);
			if (vbc)
				src0 = LL_define_VB(vbc, src0);
		}
		c--;
	} while (--len);
	return src0;
}

//
// Lilte-Linker, srcとdestの解析結果を出力する
// (なおsrcに書き込みをするので要注意)
//
UCHAR *LL(UCHAR *src0, UCHAR *src1, UCHAR *dest0, UCHAR *dest1)
{
	struct STR_SUBSECTION *subsect1;
	unsigned int l0, min, max, unsolved, unsolved0;
	struct STR_LL_VB virtualbyte[8], *vba, *vbb, *vbc;
	signed int times_count;
	UCHAR *s, *times_src0, *times_dest0, c, len, range;

	std::array<STR_LABEL, nask_maxlabels> label0;
	STR_LABEL labels;
	STR_VALUE labels_value;
	labels_value.flags = 0;
	labels.value = labels_value;  // enableを消す
	labels.define = nullptr;      // C++らしく
	std::fill(std::begin(label0), std::end(label0), labels);

	STR_LABEL* label = label0.data();
	std::shared_ptr<STR_SUBSECTION> subsect0(new STR_SUBSECTION());
	std::unique_ptr<STR_VALUE> value(new STR_VALUE());
	std::unique_ptr<STR_VALUE> labelvalue(new STR_VALUE());

	/* ラベル式を登録する */
//	l0 = 0; /* 現在のアドレスはl0からlc個の和 */
//	lc = 0;
//	min = max = 0; /* 次に生成するのは、variable0[l0 + lc] */

#if defined(DEBUG) && defined(TRACE) // ここまでデバッグログは出す必要ないと思うので
	for (unsolved = nask_maxlabels; unsolved >= 0; unsolved--) {
	     LOG_DEBUG("Check [%d], flags: %d, define: %d \n",
		       unsolved,
		       label0[unsolved].value.flags,
		       label0[unsolved].define);
	}
#endif

	/* 切り分ける */
	/* ラベルを検出する */
	LOG_DEBUG("detect labels \n");
	std::shared_ptr<STR_SUBSECTION> subsect = subsect0;
	subsect->sect0 = src0;
	init_value(labelvalue.get()); /* ラベル保持 */
	while (src0 < src1) {
		c = *src0;
		if (c == 0x2d) {
			/* EQU */
			LOG_DEBUG("detect label: EQU 0x%02x \n", c);
			src0++;
			calc_value(value, &src0);
			// This was written as raw pointer:
			// => label = &label0[value->min];
			// http://www.cplusplus.com/reference/memory/shared_ptr/reset/
			label = &label0[value->min];
			label->define = src0;
			src0 = skip_expr(src0);
			continue;
		}
		if (c == 0x0e) {
			/* ラベル定義 */
			LOG_DEBUG("detect label definition: 0x%02x \n", c);
			struct STR_SIGMA sigma;
			sigma.scale = 1;
			sigma.subsect = subsect.get() - subsect0.get();
			sigma.terms = 1;
			subsect->sect1 = src0;
			manually_increment(subsect);
			subsect->sect0 = src0;
			src0++;
			calc_value(value, &src0);
			label = &label0[value->min];
			label->define = src0;
			addsigma(labelvalue, sigma);
			label->value = *labelvalue;
			label->value.flags |= VFLG_ENABLE;
			continue;
		}
		if (c == 0x2c) {
			LOG_DEBUG("detect label extern: 0x%02x \n", c);
			/* externラベル宣言 */
			src0++;
			calc_value(value, &src0);
			label = &label0[value->min];
			init_value(&label->value);
		//	label->define = NULL;
			label->value.flags |= VFLG_EXTERN | VFLG_ENABLE;
			label->value.label[0] = value->min;
			label->value.scale[0] = 1;
			continue;
		}
		if (c == 0x58) {
			LOG_DEBUG("detect label ORG: 0x%02x \n", c);
			/* ORG */
			subsect->sect1 = src0;
			manually_increment(subsect);
			subsect->sect0 = src0;
			src0++;
			calc_value(labelvalue, &src0);
			continue;
		}
		LOG_DEBUG("skipcode... \n");
		src0 = LL_skipcode(src0);
	}
	subsect->sect1 = src1;
	subsect1 = subsect.get() + 1;
	auto subsect_ptr = subsect.get();

	for (subsect = subsect0; subsect_ptr < subsect1; subsect_ptr++) {
		subsect->min = 0;
		subsect->delta = INVALID_DELTA;
		subsect->unsolved = 1;
	}
	len = 0;
	c = 0;
	unsolved0 = 0x7fffffff;
	do {
		for (subsect = subsect0; subsect_ptr < subsect1; subsect_ptr++) {
			if (subsect->unsolved)
				solve_subsection(subsect.get(), len);
		}
		unsolved = 0;
		for (subsect_ptr = subsect1 - 1; subsect_ptr >= subsect0.get(); subsect_ptr--) {
			if (subsect->unsolved)
				unsolved += solve_subsection(subsect.get(), len);
		}
		c++;
		if (unsolved0 > unsolved) {
			c = 0;
			unsolved0 = unsolved;
		}
		if (c >= 3) {
			len++;
			c = 0;
		}
	} while (unsolved);

	for (subsect = subsect0; subsect_ptr < subsect1 - 1; subsect_ptr++) {
		LOG_DEBUG("sebsect++ ! \n");
		times_count = 0;
		for (src0 = subsect->sect0; src0 < subsect->sect1; ) {
			c = *src0++;
			if (dest0 + 64 >= dest1) {
	error:
				return NULL; /* buffer overrun */
			}
			if (c == 0x2d) {
				/* EQU */
				calc_value(value, &src0);
				label = &label0[value->min];
				if ((label->value.flags & VFLG_ENABLE) == 0)
					enable_label(label);
				*value = label->value;
				calcsigma(value);
				c = 0x0c;
				if (value->scale[0])
					c++;
				*dest0++ = c;
				len = 4;
				src0 = skip_expr(src0);
				goto putvalue0;
			}
			if (c == 0x0e) {
				src0 = skip_expr(src0);
				continue;
			}
			if (c == 0x0f) {
				dest0[0] = 0xf6; /* REM_8B */
				dest0[1] = *src0++;
				c = *src0++;
				label = &(label0)[get_id(c - 8, &src0, 0)];
				if ((label->value.flags & VFLG_ENABLE) == 0)
					enable_label(label);
				*value = label->value;
				calcsigma(value);
				dest0[2] = 0; /* base extern label (none) */
				dest0[3] = 0;
				dest0[4] = value->min & 0xff;
				dest0[5] = (value->min >>  8) & 0xff;
				dest0[6] = (value->min >> 16) & 0xff;
				dest0[7] = (value->min >> 24) & 0xff;
				dest0 += 8;
				if (value->scale[0]) {
					dest0[2 - 8] = value->label[0] & 0xff;
					dest0[3 - 8] = (value->label[0] >> 8) & 0xff;
				//	if (value->scale[0] != 1 || value->scale[1] != 0) {
				//		/* エラーを出力 */
				//	}
				}
				continue;
			}
			if (c == 0x2c) {
				dest0[0] = 0x2c;
				dest0[1] = src0[0];
				dest0[2] = src0[1];
				dest0[3] = src0[2];
				src0 += 3;
				dest0 += 4;
				continue;
			}
			#if (DEBUG)
				if (c < 0x30)
					goto dberr0;
			#endif
			if (c < 0x37) {
				/* 30〜37:ショートDB(定数、0〜7バイト) */
				len = c & 0x07;
	copy:
				*dest0++ = c;
				if (len) {
					times_count -= len;
					do {
						*dest0++ = *src0++;
					} while (--len);
	check_times_count:
					if (times_count == 0) {
	times_expand:
						s = skip_expr(times_src0 + 4); /* len */
						calc_value(value, &s); /* 繰り返し回数 */
						c = 0;
						value->min = times_src0[0] | times_src0[1] << 8
							| times_src0[2] << 16 | times_src0[3] << 24;
						/* ↑forceなどで食い違いが生じないようにするため */
						if (value->delta) {
							/* 不定値エラー */
							c = 1;
							*dest0++ = 0xe9;
						}
						if (value->flags & VFLG_UNDEF) {
							/* 未定義エラー */
							c = 1;
							*dest0++ = 0xec;
						}
						if (value->flags & VFLG_SLFREF) {
							/* 自己参照エラー */
							c = 1;
							*dest0++ = 0xea;
						}
						if (value->flags & VFLG_ERROR) {
							/* ラベル値一般エラー */
							c = 1;
							*dest0++ = 0xeb;
						}
						if (c != 0 || (unsigned int) value->min >= 0xfffffff0) {
							*dest0++ = 0xe6; /* TIMES error */
							dest0 = times_dest0; /* 繰り返し回数0とみなす */
							continue;
						}
						value->delta = dest0 - times_dest0;
						if (dest0 + value->delta * value->min + 64 >= dest1)
							goto error; /* 1回分余計に比較しているが、ま、いいか */
						for (dest0 = times_src0 = times_dest0; value->min > 0; value->min--) {
							s = times_src0;
							for (times_count = value->delta; times_count > 0; times_count--)
								*dest0++ = *s++;
						}
					}
	times_count0:
					times_count = 0;
				}
				continue;
			}
			if (c < 0x3c) {
				/* 式の値を置く */
				len = c - 0x37;
				range = *src0++;
				calc_value(value, &src0);
	putvalue:
				if (value->scale[0] && len == 4 && value->delta == 0
					&& (value->flags & (VFLG_UNDEF | VFLG_SLFREF | VFLG_ERROR)) == 0) {
					/* 要リロケーション(1か-1) */
					if (value->delta == 0 && value->flags)
					if (value->scale[0] == -1) {
						value->label[0] = value->label[1];
						value->scale[1] = -1;
					}
					/* ここで想定すべきケース */
					/* scale[0] == 1, scale[1] ==  0 : 絶対補正 */
					/* scale[0] == 1, scale[1] == -1 : 相対補正 */
					/* 0x2e : 絶対補正要求, 0x2f : 相対補正要求 */
					dest0[0] = 0x2e - value->scale[1];
					dest0[1] = 0x09;
					dest0[2] = value->label[0] & 0xff;
					dest0[3] = (value->label[0] >> 8) & 0xff;
					dest0 += 4;
				}
				*dest0++ = 0x30 | len; /* 31〜34 */
				/* レンジチェックなどをやる */
				/* scaleチェックもやって、必要ならリンクに必要な情報をつける */
	putvalue0:
				/* lenバイトを置く */
				dest0[0] = value->min         & 0xff;
				dest0[1] = (value->min >>  8) & 0xff;
				dest0[2] = (value->min >> 16) & 0xff;
				dest0[3] = (value->min >> 24) & 0xff;
				dest0 += len;
				if (value->delta) {
					/* 不定値エラー */
					*dest0++ = 0xe9;
				}
				if (value->flags & VFLG_UNDEF) {
					/* 未定義エラー */
					*dest0++ = 0xec;
				}
				if (value->flags & VFLG_SLFREF) {
					/* 自己参照エラー */
					*dest0++ = 0xea;
				}
				if (value->flags & VFLG_ERROR) {
					/* ラベル値一般エラー */
					*dest0++ = 0xeb;
				}
				if ((times_count -= len) == 0)
					goto times_expand;
				goto times_count0;
			//	times_count = 0;
			//	continue;
			}
			if (c == 0x58) {
				/* ORG */
				calc_value(value, &src0);
				*dest0++ = 0x5a;
				len = 4;
				goto putvalue0;
			}
			if (c == 0x59) {
				/* TIMES */
				times_dest0 = dest0;
				times_src0 = src0;
				src0 += 4;
				calc_value(value, &src0); /* len */
				/* lenの部分にエラーはないということになっている(手抜き) */
				times_count = value->min;
				src0 = skip_expr(src0);

		     		LOG_DEBUG("TIMES microcode \nvalue: %s \ntimes_count: %d \n",
					  value->to_string().c_str(), times_count);
				continue;
			}
			if (c == 0x68) {
				/* endian */
				src0++;
				continue;
			}
			#if (DEBUG)
				if (c < 0x70)
					goto dberr0;
			#endif
			if (c < 0x78) {
				/* 0x70〜0x77:virtual byte定義 */
				src0 = LL_define_VB(&virtualbyte[c & 0x07], src0);
				continue;
			}
			if (c < 0x80) {
				/* 0x78〜0x7f:virtual byte展開 */
				vba = &virtualbyte[c & 0x07];
				s = vba->expr;
				if (vba->typlen < 0x38) {
					len = vba->typlen & 0x07;
					*dest0++ = vba->typlen;
					if (len) {
						times_count -= len;
						do {
							*dest0++ = *s++;
						} while (--len);
						goto check_times_count;
					}
					continue;
				}
				len = vba->typlen - 0x37;
				range = vba->range;
				calc_value(value, &s);
				goto putvalue;
			}
			#if (DEBUG)
				if (c < 0x90)
					goto dberr0;
			#endif
			if (c <= 0x91) {
				/* microcode90 */
				/* microcode91 */
				len = src0[2]; /* z-bbb-d-aaa */
				vba = &virtualbyte[len & 0x07];
				vbb = &virtualbyte[(len >> 4) & 0x07];
				vbc = vbb + 1;
				if (c == 0x90)
					vbc = NULL;
				vba->typlen = 0x37 + (src0[1] & 0x0f);
				vba->range = 0x03; /* non-overflow */
				if (vba->typlen == 1)
					vba->range = 0x02; /* normal-signed */
				c = src0[0] & 0x0f;
				len = ((len /* z-bbb-d-aaa */ >> 7) & 0x01) + 2;
				vba->expr = src0 + 3;
				src0 = skip_expr(vba->expr);

				src0 = lccbug_LL_mc90_func(src0, vbb, vbc, len, c);
#if 0
				do {
					if (c != 0) {
						src0 = skip_mc30(src0, NULL, 1);
						if (vbc)
							src0 = skip_mc30(src0, NULL, 1);
					} else {
						src0 = LL_define_VB(vbb, src0);
						if (vbc)
							src0 = LL_define_VB(vbc, src0);
					}
					c--;
				} while (--len);
#endif
				continue;
			}
			#if (DEBUG)
				if (c < 0x94)
					goto dberr0;
			#endif
			if (c <= 0x95) {
				/* microcode94 */
				/* microcode95 */
				len = src0[2]; /* z-bbb-d-aaa */
				vba = &virtualbyte[len & 0x07];
				vbb = &virtualbyte[(len >> 4) & 0x07];
				vbc = vbb + 1;
				if (c == 0x94)
					vbc = NULL;
				vba->expr = src0 + 3;
				c = src0[0] & 0x0f;
				src0 = skip_expr(vba->expr);
				len = *src0++;
				goto outmc94_skip;
				do {
					c--;
					src0 = skip_expr(src0);
			outmc94_skip:
					if (c == 0) {
						range = *src0++;
						if (range < 0x3c) {
							vba->typlen = range;
							vba->range = *src0++;
						} else {
							vba->typlen = table98typlen[range & 0x07];
							vba->range = table98range[range & 0x07];
						}
						src0 = LL_define_VB(vbb, src0);
					} else {
						src0 = skip_mc30(src0, NULL, 0);
						src0 = skip_mc30(src0, NULL, 1);
					}
					if (vbc) {
						if (c == 0)
							src0 = LL_define_VB(vbc, src0);
						else
							src0 = skip_mc30(src0, NULL, 1);
					}
				} while (--len);
				continue;
			}
			#if (DEBUG)
				if (c < 0x98)
					goto dberr0;
			#endif
			if (c < 0x9f) {
				len = table98typlen[c & 0x07] & 0x07;
				range = table98range[c & 0x07];
				calc_value(value, &src0);
				goto putvalue;
			}
			#if (DEBUG)
				if (c < 0xe0)
					goto dberr0;
			#endif
			if (c < 0xf0) {
				/* 1バイトリマーク */
				*dest0++ = c;
				continue;
			}
			if (c < 0xf8) {
				/* nバイトリマーク */
				len = c - 0xef;
				goto copy;
			}
			#if (DEBUG)
	dberr0:
				fprintf(stderr, "LL:%02x\n", c);
				//GOL_sysabort(GO_TERM_BUGTRAP);
			#endif
		}
	}

	return dest0;
}


UCHAR *skip_mc30(UCHAR *s, UCHAR *bytes, char flag)
{
	return LL_skip_mc30(s, bytes, flag);
}

//
// unsolved数を返す
// subsect->unsolved == 0 なら呼ばないこと
//
unsigned int solve_subsection(struct STR_SUBSECTION *subsect, char force)
{
	LOG_DEBUG("solve_subsection \n");

	struct {
		signed char min, max;
	} vb[8], *vba, *vbb, *vbc;
	unsigned int unsolved = 0, min = 0, delta = 0;
	int i;
	std::unique_ptr<STR_VALUE> value(new STR_VALUE());
	std::unique_ptr<STR_VALUE> tmp(new STR_VALUE());

	UCHAR *s, *t, c, d, e, f, g, h;

	for (s = subsect->sect0; s < subsect->sect1; ) {
		c = *s++;
		if (c == 0x2d) {
			/* EQUラベル定義 */
			s = skip_expr(s);
			s = skip_expr(s);
			continue;
		}
		if (c == 0x0e) {
			/* 非EQUラベルの定義 */
			/* 今は手抜きなので取り合えず無視 */
			s = skip_expr(s);
			continue;
			/* 本来はこれは来ない(？) */
		}
		if (c == 0x0f) {
			s += s[1] - (0x08 - 3); /* GLOBAL用変換 */
			continue;
		}
		if (c == 0x2c) {
			s = skip_expr(s);
			continue;
		}
		#if (DEBUG)
			if (c < 0x30)
				goto dberr0;
		#endif
		if (c < 0x3c) {
skipmc30:
			/* 30〜37:ショートDB(定数、0〜7バイト) */
			/* 式をDB〜DDで置く */
			s = skip_mc30(s - 1, &c, 1);
			min += c;
			continue;
		}
		if (c == 0x58) {
			/* ORG */
			s = skip_expr(s);
			continue;
		}
		if (c == 0x59) {
			/* TIMES */
			LOG_DEBUG("TIMES microcode 0x%02x \n", c);
			t = s;
			i = s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
			s += 4;
			calc_value(value, &s); /* len */
			/* lenにエラーはないという前提(プログラムカウンタを含まない定数式) */
			if ((unsigned int) i < 0xfffffff0) {
				min += (i - 1) * value->min; /* iは繰り返し回数 */
				s = skip_expr(s);
				continue;
			}
			i = value->min;
			calc_value(value, &s); /* 繰り返し回数 */
			if (value->flags & (VFLG_SLFREF | VFLG_UNDEF)) /* 自己参照エラー */
				goto mc59_force;
			if (value->flags & VFLG_ERROR) {
				/* ラベル値一般エラー */
				if (force >= 2)
					goto mc59_force;
				delta = INVALID_DELTA;
				unsolved++;
				continue;
			}
			min += i * (value->min - 1);
			if (value->delta == 0) {
				t[0] = value->min         & 0xff;
				t[1] = (value->min >>  8) & 0xff;
				t[2] = (value->min >> 16) & 0xff;
				t[3] = (value->min >> 24) & 0xff;
				continue;
			}
			if (force < 2) {
				delta += i * value->delta;
				unsolved++;
				continue;
			}
	mc59_force:
			t[0] = 0xfe; /* force */
		//	t[1] = 0xff;
		//	t[2] = 0xff;
		//	t[3] = 0xff;
			continue;
		}
		if (c == 0x68) {
			/* endian */
			s++;
			continue;
		}
		#if (DEBUG)
			if (c < 0x70)
				goto dberr0;
		#endif
		if (c < 0x78) {
			/* 0x70〜0x77:virtual byte定義 */
			vba = &vb[c & 0x07];
			s = skip_mc30(s, reinterpret_cast<UCHAR*>(&vba->min), 1);
			vba->max = vba->min;
			continue;
		}
		if (c < 0x80) {
			/* 0x78〜0x7f:virtual byte展開 */
			vba = &vb[c & 0x07];
			min += vba->min;
			delta += vba->max - vba->min;
			continue;
		}
		#if (DEBUG)
			if (c < 0x90)
				goto dberr0;
		#endif
		if (c <= 0x91) {
			/* microcode90, microcode91 */
			/* [90 decision z-bbb-d-aaa expr] [3x (d,w)] [3x (b)] ([3x (z)]) */
			/* [91 decision z-bbb-d-aaa expr] [3x 3x (d,w)] [3x 3x (b)] ([3x 3x (z)]) */
			/* 条件式はvbaへ */
			/* decisionは2バイト。選択番号とbbbb-aaaa */
			d = c;
			c = s[2]; /* z-bbb-d-aaa */
			vba = &vb[c        & 0x07];
			vbb = &vb[(c >> 4) & 0x07];
			vbc = vbb + 1;
			if (d == 0x90)
				vbc = NULL;
			if (((e = s[0]) & 0x0f) != 0x0f) {
				/* 既に解決済み */
				d = s[1];
				s = skip_expr(s + 3);
				vba->min = vba->max = d        & 0x0f;
				vbb->min = vbb->max = (d >> 4) & 0x0f;
			//	if (vbc)
			//		vbc->min = vbc->max = (e >> 4) & 0x0f;
				d = 2;
				if (c & 0x80)
					d = 3;
				if (vbc) {
					vbc->min = vbc->max = (e >> 4) & 0x0f;
					d *= 2;
				}
				do {
					s = skip_mc30(s, NULL, 1);
				} while (--d);
				continue;
			}
			t = s;
			s += 3;
			calc_value(value, &s);
			d = 1 | 2 | 4; /* unknown */
			if ((value->flags & VFLG_ERROR) == 0) {
				i = value->min + value->delta; /* max */
				if (value->scale[0])
					d = 1; /* word(dword) */
				else if (value->min == 0 && i == 0)
					d = 4; /* zero */
				else if (-128 <= value->min && i <= 127) {
					d = 2; /* byte */
					if (value->min <= 0 && 0 <= i)
						d = 2 | 4; /* byte | zero */
				} else {
					d = 1; /* word(dword) */
					if (value->min <= 127 && -128 <= i) {
						d = 1 | 2; /* word | byte */
						if (value->min <= 0 && 0 <= i)
							d = 1 | 2 | 4; /* word | byte | zero */
					}
				}
			}
			if (force) {
				if (d & 1)
					d = 1;
				else
					d = 2;
			}
			if ((c & 0x80) == 0) { /* zeroなし */
				if ((d &= (1 | 2)) == 0)
					d = 2; /* byte */
			}
			vba->min = vbb->min =  127;
			vba->max = vbb->max = -128;
			if (vbc) {
				vbc->min =  127;
				vbc->max =  -128;
			}
			for (e = 1; e <= 4; e <<= 1) {
				if (e == 4 && (c & 0x80) == 0)
					break;
				s = skip_mc30(s, &f, 1);
				if (vbc)
					s = skip_mc30(s, &g, 1);
				if (d & e) {
					if (vbb->min > f)
						vbb->min = f;
					if (vbb->max < f)
						vbb->max = f;
					if (vbc) {
						if (vbc->min > g)
							vbc->min = g;
						if (vbc->max < g)
							vbc->max = g;
					}
					f = 2;
					if (c & 0x08)
						f = 4;
					if (e == 2)
						f = 1;
					if (e == 4)
						f = 0;
					if (vba->min > f)
						vba->min = f;
					if (vba->max < f)
						vba->max = f;
				}
			}
			if ((d & (d - 1)) == 0) {
				c = 0;
				if (d == 2)
					c = 1;
				if (d == 4)
					c = 2;
				t[0] = c;
				if (vbc)
					t[0] |= vbc->min << 4;
				t[1] = vba->min | vbb->min << 4;
			} else
				unsolved++;
			continue;
		}
		#if (DEBUG)
			if (c < 0x94)
				goto dberr0;
		#endif
		if (c <= 0x95) {
			/* microcode94, microcode95 */
			/* [94 decision 0-bbb-0-aaa expr] [len] [def-a def-b] ([expr def-a def-b] ...) */
			/* [95 decision 0-bbb-0-aaa expr] [len] [def-a def-b def-c] ([expr def-a def-b def-c] ...) */
			/* def-a は 0x98〜0x9e (値は条件式) */
			/* lenは最低1(caseなし), 最初は無条件定義, 後続は一致したら定義, 一致が何度もあれば、最後が有効 */
			d = c;
			c = s[2]; /* 0-bbb-0-aaa */
			vba = &vb[c        & 0x07];
			vbb = &vb[(c >> 4) & 0x07];
			vbc = vbb + 1;
			if (d == 0x94)
				vbc = NULL;
			if (((g = s[0]) & 0x0f) != 0x0f) {
				/* 既に解決済み */
				d = s[1];
				s = skip_expr(s + 3);
				e = *s++; /* len */
				vba->min = vba->max = d        & 0x0f;
				vbb->min = vbb->max = (d >> 4) & 0x0f;
				if (vbc)
					vbc->min = vbc->max = (g >> 4) & 0x0f;
				s = skip_mc30(s, NULL, 0);
				s = skip_mc30(s, NULL, 1);
				if (vbc)
					s = skip_mc30(s, NULL, 1);
				while (--e) {
					s = skip_expr(s);
					s = skip_mc30(s, NULL, 0);
					s = skip_mc30(s, NULL, 1);
					if (vbc)
						s = skip_mc30(s, NULL, 1);
				}
				continue;
			}
			t = s;
			s += 3;

			calc_value(value, &s);
			c = *s++; /* len */
			s = skip_mc30(s, reinterpret_cast<UCHAR*>(&vba->min), 0);
			s = skip_mc30(s, reinterpret_cast<UCHAR*>(&vbb->min), 1);
			vba->max = vba->min;
			vbb->max = vbb->min;
			if (vbc) {
				s = skip_mc30(s, reinterpret_cast<UCHAR*>(&vbc->min), 1);
				vbc->max = vbc->min;
			}
			f = 0;
			if (force == 0) {
				if (value->delta != 0 || (value->flags & VFLG_ERROR) != 0)
					f = 0xff;
				for (g = 1; g < c; g++) {
					/* どうやって等しいことを見極めるのか？ */
					/* 面倒なので、全てが確定するまで、decideしない */

					calc_value(tmp, &s);
					if (tmp->delta != 0 || (tmp->flags & VFLG_ERROR) != 0)
						f = 0xff;
					s = skip_mc30(s, &d, 0);
					s = skip_mc30(s, &e, 1);
					if (vbc)
						s = skip_mc30(s, &h, 1);
					if (f != 0xff) {
						if ((value->min ^ tmp->min |
							value->scale[0] ^ tmp->scale[0] | value->label[0] ^ tmp->label[0] |
							value->scale[1] ^ tmp->scale[1] | value->label[1] ^ tmp->label[1]) == 0) {
							vba->min = vba->max = d;
							vbb->min = vbb->max = e;
							if (vbc)
								vbc->min = vbc->max = h;
							f = g;
						}
					} else {
						if (vba->min > d)
							vba->min = d;
						if (vba->max < d)
							vba->max = d;
						if (vbb->min > e)
							vbb->min = e;
						if (vbb->max < e)
							vbb->max = e;
						if (vbc) {
							if (vbb->min > h)
								vbb->min = h;
							if (vbb->max < h)
								vbb->max = h;
						}
					}
				}
			}
			if (f != 0xff) {
				t[0] = f;
				if (vbc)
					t[0] |= vbc->min << 4;
				t[1] = vba->min | vbb->min << 4;
			} else
				unsolved++;
			continue;
		}
		#if (DEBUG)
			if (c < 0x98)
				goto dberr0;
		#endif
		if (c < 0x9f)
			goto skipmc30;
		#if (DEBUG)
			if (c < 0xe0)
				goto dberr0;
		#endif
		if (c < 0xf0)
			continue; /* 1バイトリマーク */
		if (c < 0xf8) {
			/* nバイトリマーク */
			s += c - 0xef;
			continue;
		}
		#if (DEBUG)
dberr0:
			fprintf(stderr, "solve_subsect:%02X\n", c);
			//GOL_sysabort(GO_TERM_BUGTRAP);
		#endif


/*
  [90(91も同等)] [disp式] [ba] : aは0〜7, bも0〜7 : virtual byte番号 : B=8は未使用を意味する
  aのbit3は、Dビット。aについては、式が入る。レンジチェックは03タイプ。
  [90] [disp式] [09] [31, 00-nnn-r/m] [31 01-nnn-r/m] [31 10-nnn-r/m]
  [bits 32]でのPUSHはこうなる:[91] [imm式] [3a] [31 6a] [31 68]
  94も似たような形式に変更(aの自動定義はない)。
  [94] [式] [ba] [case] [def0] [def1] [case] [def0] [def1] ...
  また以下を定義
  [98] = [38 00 08 00] UCHAR
  [99] = [38 02 08 00] SCHAR
  [9a] = [39 00 08 00] USHORT
  [9b] = [39 03 08 00] non-overflow SHORT
  [9c] = [3b 00 08 00] UINT
  [9d] = [3b 03 08 00] non-overflow INT
  [9e] = [38 03 08 00] non-overflow CHAR

  ・70〜77:可変長バイト宣言(文法上では40〜4bが後続することを許すが、サポートしていない。許されるのは30〜3f)
  ・78〜7f:可変長バイト参照

  ・80〜8f:LLが内部処理用に使う
	80〜83:variable参照(1〜4バイト)
	88〜8f:sum(variable), (1〜4, 1〜4) : 最初は項数-1, 次は最初の番号

  ・90:0-B-elseバウンダリーif
  ・91:B-elseバウンダリーif
  ・92:0BWDバウンダリーif
  ・93:else
  ・選択的定数if
*/

	}
	if (delta > INVALID_DELTA)
		delta = INVALID_DELTA;
	subsect->min = min;
	subsect->delta = delta;
	subsect->unsolved = unsolved;

	return unsolved;
}

void calcsigma(std::unique_ptr<STR_VALUE>& value)
/* sigmaを全て展開してしまう */
{
	int i, j, s, t, min;
	unsigned int delta;
	struct STR_SUBSECTION *subsect;
	for (i = 0; i < MAXSIGMAS; i++) {
		if ((s = value->sigma[i].scale) == 0)
		//	continue;
			break;
		value->sigma[i].scale = 0;
		if ((t = value->sigma[i].terms) == 0)
			continue;
		value->sigma[i].terms = 0;
		subsect = &subsect0[value->sigma[i].subsect];
		value->sigma[i].subsect = 0;
		min = delta = 0;
		do {
			min += subsect->min;
			delta += subsect->delta;
			if (delta > INVALID_DELTA)
				delta = INVALID_DELTA;
			subsect++;
		} while (--t);
		if (delta >= INVALID_DELTA) {
			value->delta = INVALID_DELTA;
		}
		if (value->delta >= INVALID_DELTA) {
			value->flags |= VFLG_ERROR;
			continue;
		}
		if (s > 0) {
			value->min += s * min;
			value->delta += s * delta;
		} else {
			value->min += s * (min + delta);
			value->delta -= s * delta;
		}
	}
	return;
}

void addsigma(std::unique_ptr<STR_VALUE>& value, struct STR_SIGMA sigma)
{
	int i, j;
retry:
	for (i = 0; i < MAXSIGMAS && value->sigma[i].scale != 0; i++) {
		if (sigma.subsect + sigma.terms <= value->sigma[i].subsect)
			break;
		if (value->sigma[i].subsect + value->sigma[i].terms <= sigma.subsect)
			continue;
	add:
		if (value->sigma[i].subsect == sigma.subsect && value->sigma[i].terms <= sigma.terms) {
	add2:
			sigma.terms -= value->sigma[i].terms;
			sigma.subsect += value->sigma[i].terms;
			if ((value->sigma[i].scale += sigma.scale) == 0) {
	del:
				for (j = i; j <= MAXSIGMAS - 2; j++)
					value->sigma[j] = value->sigma[j + 1];
				value->sigma[MAXSIGMAS - 1].scale = 0;
				value->sigma[MAXSIGMAS - 1].subsect = 0;
				value->sigma[MAXSIGMAS - 1].terms = 0;
			}
	check:
			do {
				if (value->sigma[i].scale != 0) {
					if (i > 0) {
						if (value->sigma[i - 1].scale == value->sigma[i].scale) {
							if (value->sigma[i - 1].subsect + value->sigma[i - 1].terms == value->sigma[i].subsect) {
								value->sigma[i - 1].terms += value->sigma[i].terms;
								goto del;
							}
						}
					}
				}
			} while (++i < MAXSIGMAS);
			if (sigma.terms > 0)
				goto retry;
			return;
		}
		if (value->sigma[MAXSIGMAS - 1].scale != 0) {
			calcsigma(value);
			goto retry;
		}
		for (j = MAXSIGMAS - 2; i <= j; j--)
			value->sigma[j + 1] = value->sigma[j];
		if (value->sigma[i].subsect == sigma.subsect) {
			value->sigma[i].terms = sigma.terms;
			value->sigma[i + 1].subsect += sigma.terms;
			value->sigma[i + 1].terms -= sigma.terms;
			goto add2;
		}
#if 0
		if (sigma.subsect < value->sigma[i].subsect) {
			value->sigma[i].terms = j = value->sigma[i].subsect - sigma.subsect;
			value->sigma[i].subsect = sigma.subsect;
			value->sigma[i].scale = sigma.scale;
			sigma.terms -= j;
			sigma.subsect += j;
			i++;
			value->sigma[i].subsect = sigma.subsect;
			value->sigma[i].terms -= j;
			goto add;
		}
	//	if (value->sigma[i].subsect < sigma.subsect) {
			value->sigma[i].terms = j = sigma.subsect - value->sigma[i].subsect;
			i++;
			value->sigma[i].subsect = sigma.subsect;
			value->sigma[i].terms -= j;
			goto add;
	//	}
#endif
		if (sigma.subsect < value->sigma[i].subsect) {
			value->sigma[i].terms = j = value->sigma[i].subsect - sigma.subsect;
			value->sigma[i].subsect = sigma.subsect;
			value->sigma[i].scale = sigma.scale;
			sigma.terms -= j;
			sigma.subsect += j;
		} else {
			value->sigma[i].terms = j = sigma.subsect - value->sigma[i].subsect;
		}
		i++;
		value->sigma[i].subsect = sigma.subsect;
		value->sigma[i].terms -= j;
		goto add;
	}
	if (value->sigma[MAXSIGMAS - 1].scale != 0) {
	     	calcsigma(value);
		goto retry;
	}
	for (j = MAXSIGMAS - 2; i <= j; j--)
		value->sigma[j + 1] = value->sigma[j];
	value->sigma[i] = sigma;
	sigma.terms = 0;
	goto check;
}
