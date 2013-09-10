int is_whitespace(int c)
{
	switch (c)
	{
	case ' ':
	case '\t':
	case '\r':
	case '\n':
		return 1;
	}
	return 0;
}

int is_sidchr(c)
{
	if (c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
		return 1;
	return 0;
}

int is_idchr(int c)
{
	if (c >= '0' && c <= '9')
		return 1;
	return is_sidchr(c);
}

int is_ep(int c)
{
	if (c == 'e' || c == 'E' || c == 'p' || c == 'P')
		return 1;
	return 0;
}

int is_hex(int c)
{
	if (c >= 'a' && c <= 'f')
		return 1;
	if (c >= 'A' && c <= 'F')
		return 1;
	if (c >= '0' && c <= '9')
		return 1;
	return 0;
}

int is_dec(int c)
{
	if (c >= '0' && c <= '9')
		return 1;
	return 0;
}

