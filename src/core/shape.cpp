#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned short mychar;
typedef unsigned char mybool;

enum Form {
	Invalid = 0x0,
	UnknownForm = Invalid,
	Consonant,
	Nukta,
	Halant,
	Matra,
	VowelMark,
	StressMark,
	IndependentVowel,
	LengthMark,
	Control,
	Other
};

static const unsigned char indicForms[0xa00-0x980] = {

		// Bengali
		Invalid, VowelMark, VowelMark, VowelMark,
		Invalid, IndependentVowel, IndependentVowel, IndependentVowel,
		IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,
		IndependentVowel, Invalid, Invalid, IndependentVowel,

		IndependentVowel, Invalid, Invalid, IndependentVowel,
		IndependentVowel, Consonant, Consonant, Consonant,
		Consonant, Consonant, Consonant, Consonant,
		Consonant, Consonant, Consonant, Consonant,

		Consonant, Consonant, Consonant, Consonant,
		Consonant, Consonant, Consonant, Consonant,
		Consonant, Invalid, Consonant, Consonant,
		Consonant, Consonant, Consonant, Consonant,

		Consonant, Invalid, Consonant, Invalid,
		Invalid, Invalid, Consonant, Consonant,
		Consonant, Consonant, UnknownForm, UnknownForm,
		Nukta, Other, Matra, Matra,

		Matra, Matra, Matra, Matra,
		Matra, Invalid, Invalid, Matra,
		Matra, Invalid, Invalid, Matra,
		Matra, Halant, Consonant, UnknownForm,

		Invalid, Invalid, Invalid, Invalid,
		Invalid, Invalid, Invalid, VowelMark,
		Invalid, Invalid, Invalid, Invalid,
		Consonant, Consonant, Invalid, Consonant,

		IndependentVowel, IndependentVowel, VowelMark, VowelMark,
		Other, Other, Other, Other,
		Other, Other, Other, Other,
		Other, Other, Other, Other,

		Consonant, Consonant, Other, Other,
		Other, Other, Other, Other,
		Other, Other, Other, Other,
		Other, Other, Other, Other
};

enum Position {
	None,
	Pre,
	Above,
	Below,
	Post,
	Split,
	Base,
	Reph,
	Vattu,
	Inherit
};

static const unsigned char indicPosition[0xa00-0x980] = {

		// Bengali
		None, Above, Post, Post,
		None, None, None, None,
		None, None, None, None,
		None, None, None, None,

		None, None, None, None,
		None, None, None, None,
		None, None, None, None,
		None, None, None, None,

		None, None, None, None,
		None, None, None, None,
		None, None, None, None,
		Below, None, None, Post,

		Below, None, None, None,
		None, None, None, None,
		None, None, None, None,
		Below, None, Post, Pre,

		Post, Below, Below, Below,
		Below, None, None, Pre,
		Pre, None, None, Split,
		Split, Below, None, None,

		None, None, None, None,
		None, None, None, Post,
		None, None, None, None,
		None, None, None, None,

		None, None, Below, Below,
		None, None, None, None,
		None, None, None, None,
		None, None, None, None,

		Below, None, None, None,
		None, None, None, None,
		None, None, None, None,
		None, None, None, None
};

static inline Form form(unsigned short uc) {
	if (uc < 0x980 || uc > 0x9ff) {
		if (uc == 0x25cc)
			return Consonant;
		if (uc == 0x200c || uc == 0x200d)
			return Control;
		return Other;
	}
	return (Form)indicForms[uc-0x980];
}

static inline Position indic_position(unsigned short uc) {
	if (uc < 0x980 || uc > 0x9ff)
		return None;
	return (Position) indicPosition[uc-0x980];
}


enum IndicScriptProperties {
	HasReph = 0x01,
	HasSplit = 0x02
};

const uint scriptProperties =

		// Bengali,
		HasReph|HasSplit
		;

struct IndicOrdering {
	Form form;
	Position position;
};

static const IndicOrdering bengali_order [] = {
		{ Consonant, Below },
		{ Matra, Below },
		{ Matra, Above },
		{ Consonant, Reph },
		{ VowelMark, Above },
		{ Consonant, Post },
		{ Matra, Post },
		{ VowelMark, Post },
		{ (Form)0, None }
};



static const IndicOrdering * const indic_order =

		bengali_order // Bengali

		;



// vowel matras that have to be split into two parts.
static const unsigned short split_matras[]  = {
		//  matra, split1, split2, split3

		// bengalis
		0x9cb, 0x9c7, 0x9be, 0x0,
		0x9cc, 0x9c7, 0x9d7, 0x0
};

static inline void splitMatra(unsigned short *reordered, int matra, int &len)
{
	unsigned short matra_uc = reordered[matra];
	//qDebug("matra=%d, reordered[matra]=%x", matra, reordered[matra]);

	const unsigned short *split = split_matras;
	while (split[0] < matra_uc)
		split += 4;

	assert(*split == matra_uc);
	++split;

	int added_chars = split[2] == 0x0 ? 1 : 2;

	memmove(reordered + matra + added_chars, reordered + matra, (len-matra)*sizeof(unsigned short));
	reordered[matra] = split[0];
	reordered[matra+1] = split[1];
	if(added_chars == 2)
		reordered[matra+2] = split[2];
	len += added_chars;
}

static bool indic_shape_syllable(mychar * text,int len,mychar * reordered,
		int * newlength)
{

	const unsigned short script_base = 0x0980;
	const unsigned short ra = script_base + 0x30;
	const unsigned short ba = 0x09ac;
	const unsigned short ya = 0x09af;
	const unsigned short halant = script_base + 0x4d;
	const unsigned short nukta = script_base + 0x3c;
	bool control = false;

	//	reordered = (mychar *)malloc((len+4) * sizeof(mychar));
	mychar * position = (mychar *)malloc((len+4) * sizeof(mychar));

	unsigned char properties = scriptProperties;


	memcpy(reordered, text, len*sizeof(mychar));

	if (reordered[len-1] == 0x200c) // zero width non joiner
		len--;

	int i;
	int base = 0;
	int reph = -1;

	if (len != 1) {
		mychar *uc = reordered;
		bool beginsWithRa = false;

		// Rule 1: find base consonant
		//
		// The shaping engine finds the base consonant of the
		// syllable, using the following algorithm: starting from the
		// end of the syllable, move backwards until a consonant is
		// found that does not have a below-base or post-base form
		// (post-base forms have to follow below-base forms), or
		// arrive at the first consonant. The consonant stopped at
		// will be the base.
		//
		//  * If the syllable starts with Ra + H (in a script that has
		//    'Reph'), Ra is excluded from candidates for base
		//    consonants.
		//
		// * In Kannada and Telugu, the base consonant cannot be
		//   farther than 3 consonants from the end of the syllable.
		// #### replace the HasReph property by testing if the feature exists in the font!
		if (form(*uc) == Consonant || form(*uc) == IndependentVowel) {
			if ((properties & HasReph) && (len > 2) &&
					(*uc == ra || *uc == 0x9f0) && *(uc+1) == halant)
				beginsWithRa = true;

			if (beginsWithRa && form(*(uc+2)) == Control)
				beginsWithRa = false;

			base = (beginsWithRa ? 2 : 0);

			int lastConsonant = 0;
			int matra = -1;
			// we remember:
			// * the last consonant since we need it for rule 2
			// * the matras position for rule 3 and 4

			// figure out possible base glyphs
			memset(position, 0, len);
			for (i = base; i < len; ++i) {
				position[i] = form(uc[i]);
				if (position[i] == Consonant)
					lastConsonant = i;
				else if (matra < 0 && position[i] == Matra)
					matra = i;
			}
			int skipped = 0;
			Position pos = Post;
			for (i = len-1; i > base; i--) {
				if (position[i] != Consonant && position[i] != Control )
					continue;

				Position charPosition = indic_position(uc[i]);
				if (pos == Post && charPosition == Post) {
					pos = Post;
				} else if ((pos == Post || pos == Below) && charPosition == Below) {
					pos = Below;
				} else {
					base = i;
					break;
				}
				++skipped;
			}

			// Rule 2:
			//
			// If the base consonant is not the last one, Uniscribe
			// moves the halant from the base consonant to the last
			// one.
			if (lastConsonant > base) {
				int halantPos = 0;
				if (uc[base+1] == halant && uc[base+2] != ra && uc[base+2]!=ya && uc[base+2]!=ba)
					halantPos = base + 1;
				else if (uc[base+1] == nukta && uc[base+2] == halant)
					halantPos = base + 2;
				if (halantPos > 0) {
					for (i = halantPos; i < lastConsonant; i++)
						uc[i] = uc[i+1];
					uc[lastConsonant] = halant;
				}
			}

			// Rule 3:
			//
			// If the syllable starts with Ra + H, Uniscribe moves
			// this combination so that it follows either:

			// * the post-base 'matra' (if any) or the base consonant
			//   (in scripts that show similarity to Devanagari, i.e.,
			//   Devanagari, Gujarati, Bengali)
			// * the base consonant (other scripts)
			// * the end of the syllable (Kannada)

			Position matra_position = None;
			if (matra > 0)
				matra_position = indic_position(uc[matra]);

			if (beginsWithRa && base != 0) {
				int toPos = base+1;
				if (toPos < len && uc[toPos] == nukta)
					toPos++;
				if (toPos < len && uc[toPos] == halant)
					toPos++;
				if (toPos < len && uc[toPos] == 0x200d)
					toPos++;
				if (toPos < len-1 && uc[toPos] == ra && uc[toPos+1] == halant)
					toPos += 2;
				if (matra_position == Post || matra_position == Split) {
					toPos = matra+1;
					matra -= 2;
				}

				for (i = 2; i < toPos; i++)
					uc[i-2] = uc[i];
				uc[toPos-2] = ra;
				uc[toPos-1] = halant;
				base -= 2;
				if (properties & HasReph)
					reph = toPos-2;
			}

			// Rule 4:

			// Uniscribe splits two- or three-part matras into their
			// parts. This splitting is a character-to-character
			// operation).
			//
			//      Uniscribe describes some moving operations for these
			//      matras here. For shaping however all pre matras need
			//      to be at the beginning of the syllable, so we just move
			//      them there now.
			if (matra_position == Split) {
				splitMatra(uc, matra, len);
				// Handle three-part matras (0xccb in Kannada)
				matra_position = indic_position(uc[matra]);
			}

			if (matra_position == Pre) {
				unsigned short m = uc[matra];
				while (matra--)
					uc[matra+1] = uc[matra];
				uc[0] = m;
				base++;
			}
		}

		// Rule 5:
		//
		// Uniscribe classifies consonants and 'matra' parts as
		// pre-base, above-base (Reph), below-base or post-base. This
		// classification exists on the character code level and is
		// language-dependent, not font-dependent.
		for (i = 0; i < base; ++i)
			position[i] = Pre;
		position[base] = Base;
		for (i = base+1; i < len; ++i) {
			position[i] = indic_position(uc[i]);
			// #### replace by adjusting table
			if (uc[i] == nukta || uc[i] == halant)
				position[i] = Inherit;
		}
		if (reph > 0) {
			// recalculate reph, it might have changed.
			for (i = base+1; i < len; ++i)
				if (uc[i] == ra)
					reph = i;
			position[reph] = Reph;
			position[reph+1] = Inherit;
		}

		// all reordering happens now to the chars after the base
		int fixed = base+1;
		if (fixed < len && uc[fixed] == nukta)
			fixed++;
		if (fixed < len && uc[fixed] == halant)
			fixed++;
		if (fixed < len && uc[fixed] == 0x200d)
			fixed++;

		// we continuosly position the matras and vowel marks and increase the fixed
		// until we reached the end.
		const IndicOrdering *finalOrder = indic_order;

		int toMove = 0;
		while (finalOrder[toMove].form && fixed < len-1) {
			for (i = fixed; i < len; i++) {
				//                         << "position=" << position[i];
				if (form(uc[i]) == finalOrder[toMove].form &&
						position[i] == finalOrder[toMove].position) {
					// need to move this glyph
					int to = fixed;
					if (i < len-1 && position[i+1] == Inherit) {
						unsigned short ch = uc[i];
						unsigned short ch2 = uc[i+1];
						unsigned char pos = position[i];
						for (int j = i+1; j > to+1; j--) {
							uc[j] = uc[j-2];
							position[j] = position[j-2];
						}
						uc[to] = ch;
						uc[to+1] = ch2;
						position[to] = pos;
						position[to+1] = pos;
						fixed += 2;
					} else {
						unsigned short ch = uc[i];
						unsigned char pos = position[i];
						for (int j = i; j > to; j--) {
							uc[j] = uc[j-1];
							position[j] = position[j-1];
						}
						uc[to] = ch;
						position[to] = pos;
						fixed++;
					}
				}
			}
			toMove++;
		}

	}

	if (reph > 0) {
		// recalculate reph, it might have changed.
		for (i = base+1; i < len; ++i)
			if (reordered[i] == ra)
				reph = i;
	}

	// now we have the syllable in the right order, and can start running it through open type.

//	for (i = 0; i < len; ++i)
//		control |= (form(reordered[i]) == Control);

	*newlength = len;
	return true;
}

/* syllables are of the form:

   (Consonant Nukta? Halant)* Consonant Matra? VowelMark? StressMark?
   (Consonant Nukta? Halant)* Consonant Halant
   IndependentVowel VowelMark? StressMark?

   We return syllable boundaries on invalid combinations aswell
 */
static int indic_nextSyllableBoundary(const mychar *s, int start, int end)
{
	const mychar *uc = s+start;

	int pos = 0;
	Form state = form(uc[pos]);
	pos++;

	if (state != Consonant && state != IndependentVowel) {
		if (state != Other)
		goto finish;
	}

	while (pos < end - start) {
		Form newState = form(uc[pos]);
		switch(newState) {
		case Control:
			newState = state;
			if (state == Halant)
				break;
			// the control character should be the last char in the item
			++pos;
			goto finish;
		case Consonant:
			if (state == Halant)
				break;
			goto finish;
		case Halant:
			if (state == Nukta || state == Consonant)
				break;
			// Bengali has a special exception allowing the combination Vowel_A/E + Halant + Ya
			if (pos == 1 &&
					(uc[0] == 0x0985 || uc[0] == 0x098f))
				break;

			goto finish;
		case Nukta:
			if (state == Consonant)
				break;
			goto finish;
		case StressMark:
			if (state == VowelMark)
				break;
			// fall through
		case VowelMark:
			if (state == Matra || state == LengthMark || state == IndependentVowel)
				break;
			// fall through
		case Matra:
			if (state == Consonant || state == Nukta)
				break;
			if (state == Matra) {
				// ### needs proper testing for correct two/three part matras
				break;
			}
			// ### not sure if this is correct. If it is, does it apply only to Bengali or should
			// it work for all Indic languages?
			// the combination Independent_A + Vowel Sign AA is allowed.
			if (uc[pos] == 0x9be && uc[pos-1] == 0x985)
				break;
			goto finish;

		case LengthMark:
			if (state == Matra) {
				// ### needs proper testing for correct two/three part matras
				break;
			}
		case IndependentVowel:
		case Invalid:
		case Other:
			goto finish;
		}
		state = newState;
		pos++;
	}
	finish:
	return pos+start;
}
enum indicCharClassesRange {
	IndicFirstChar = 0x980,
	IndicLastChar  = 0x9ff
};
mybool isindic(mychar ch) {
	return (ch >= IndicFirstChar && ch <= IndicLastChar);
}

int mygsub(mychar * text, int oldlen, int * newlength);

int reorder(const void *vtext, size_t len, mychar * reorder) {
	size_t i;
	mychar * text = (mychar *)vtext;
	int indic=0,cnt=0;
	len = int(len/2);
	for (i=0;i<len && indic==0;i++) {
		if (isindic(text[i])) {
			if(++cnt > 0)
				indic = 1;
		} else {
			cnt = 0;
		}
	}
	if(indic == 0)
		return 0;

	size_t start=0, count = 0;
	while (start < len) {
		if(!isindic(text[start])) {
			reorder[count++] = text[start++];
			continue;
		}
		int send = indic_nextSyllableBoundary(text, start, len);
		int newlength;

		if (!indic_shape_syllable(text+start,send-start,reorder+count,&newlength))
			return 0;

		start = send;
		count += newlength;
	}

	int newlength=count;
	int ll = 0,ret=2;

	while ((ret=mygsub(reorder,newlength, &newlength))==2 &&
			ll < 10) {
	}

	if(ret==0)
		return 0;
	count = newlength;
	reorder[count] = '\0';

	return count*2;
}
