class Solution{
public:
	int strStr(const string &haystack, const string ){
		if (needle[0]=='\0') return 0;
		for(int i=0;haystack[i] != '\0';i++)
		{
			bool isMatched = true;
			for (int j = 0; needle[j]!='\0';j++)
			{
				if(haystack[i+j]==0)return -1;
				if(haystack[i+j]!= needle[j]){
					isMatched = false; break;
				}
			}
			if(isMatched) return i;
		}
		return -1;
	}
};