/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oakyol <oakyol@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/03 00:29:53 by oakyol            #+#    #+#             */
/*   Updated: 2025/08/11 00:14:46 by oakyol           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/minishell.h"
#include "../libft/libft.h"
#include <stdlib.h>

char	*get_env_value(t_ms *ms, const char *name)
{
	int		i;
	size_t	len;
	char	*value;
	char	*quoted;

	i = 0;
	len = ft_strlen(name);
	if (!name || name[0] == '\0')
		return (gc_strdup(ms, "$"));
	while (ms->env[i])
	{
		if (!ft_strncmp(ms->env[i], name, len) && ms->env[i][len] == '=')
		{
			value = &ms->env[i][len + 1];
			if (!ft_strcmp(value, "<") || !ft_strcmp(value, ">")
				|| !ft_strcmp(value, ">>") || !ft_strcmp(value, "<<")
				|| !ft_strcmp(value, "|"))
			{
				quoted = gc_malloc(ms, ft_strlen(value) + 3);
				if (!quoted)
					return (NULL);
				quoted[0] = '"';
				ft_memcpy(quoted + 1, value, ft_strlen(value));
				quoted[1 + ft_strlen(value)] = '"';
				quoted[2 + ft_strlen(value)] = '\0';
				return (quoted);
			}
			return (gc_strdup(ms, value));
		}
		i++;
	}
	return (gc_strdup(ms, ""));
}

static char	*ft_strjoin_free(t_ms *ms, char *s1, char *s2)
{
	char	*joined;

	joined = gc_strjoin(ms, s1, s2);
	return (joined);
}

static int	check_tilde(const char *token, t_ms *ms, char **result)
{
	char	*home;
	char	*rest;

	if (!token || token[0] != '~')
		return (0);
	if (token[1] && token[1] != '/')
		return (0);
	home = get_env_value(ms, "HOME");
	if (!home)
		return (0);
	if (token[1] == '\0')
		*result = home;
	else
	{
		rest = gc_strdup(ms, token + 1);
		*result = ft_strjoin_free(ms, home, rest);
	}
	return (1);
}

static char	*expand_token(const char *token, t_ms *ms)
{
	char	*result;
	char	*temp;
	size_t	i;
	size_t	start;
	char	q;

	if (!token)
		return (NULL);
	if (check_tilde(token, ms, &result))
		return (result);
	result = gc_strdup(ms, "");
	i = 0;
	q = 0;
	while (token[i])
	{
		/* quote aç/kapa — QUOTE'LARI KORU */
		if (!q && (token[i] == '\'' || token[i] == '"'))
		{
			q = token[i];
			result = ft_strjoin_free(ms, result, gc_strndup(ms, &token[i], 1));
			i++;
			continue ;
		}
		if (q && token[i] == q)
		{
			result = ft_strjoin_free(ms, result, gc_strndup(ms, &token[i], 1));
			q = 0;
			i++;
			continue ;
		}
		/* tek tırnak içinde: tamamen literal */
		if (q == '\'')
		{
			start = i;
			while (token[i] && token[i] != '\'')
				i++;
			temp = gc_strndup(ms, token + start, i - start);
			result = ft_strjoin_free(ms, result, temp);
			continue ;
		}
		/* $ genişlemeleri (unquoted veya " içinde) */
		if (token[i] == '$')
		{
			/* $'...' => $ basma; tek tırnaklı blok olarak işle ve tırnağı KORU */
			if (!q && token[i + 1] == '\'')
			{
				q = '\'';
				result = ft_strjoin_free(ms, result, gc_strndup(ms, &token[i + 1], 1));
				i += 2;
				continue ;
			}
			/* $"..." — istersen bunu da aynı mantıkla ele al */
			if (!q && token[i + 1] == '"')
			{
				q = '"';
				result = ft_strjoin_free(ms, result, gc_strndup(ms, &token[i + 1], 1));
				i += 2;
				continue ;
			}
			if (token[i + 1] == '?')
			{
				temp = gc_itoa(ms, ms->last_exit);
				result = ft_strjoin_free(ms, result, temp);
				i += 2;
				continue ;
			}
			if (token[i + 1] && (ft_isalnum(token[i + 1]) || token[i + 1] == '_'))
			{
				start = i + 1;
				i++;
				while (token[i] && (ft_isalnum(token[i]) || token[i] == '_'))
					i++;
				temp = gc_strndup(ms, token + start, i - start);
				temp = get_env_value(ms, temp);
				result = ft_strjoin_free(ms, result, temp);
				continue ;
			}
			/* $ + başka bir şey → literal $ */
			result = ft_strjoin_free(ms, result, gc_strdup(ms, "$"));
			i++;
			continue ;
		}
		/* düz metin bloğu */
		start = i;
		if (q == '"')
		{
			while (token[i] && token[i] != '"' && token[i] != '$')
				i++;
		}
		else
		{
			while (token[i] && token[i] != '$' && token[i] != '\'' && token[i] != '"')
				i++;
		}
		if (i > start)
		{
			temp = gc_strndup(ms, token + start, i - start);
			result = ft_strjoin_free(ms, result, temp);
		}
	}
	return (result);
}

static int	append_split_if_needed(char *token, t_ms *ms, char **res)
{
	char	*expanded;
	size_t	i;
	size_t	j;
	char	*q;
	char	*rest;

	expanded = expand_token(token, ms);
	if (expanded[0] == '\0' && token[0] != '"' && token[0] != '\'')
		return (0);
	if (token[0] == '"' || token[0] == '\'')
	{
		res[0] = expanded;
		return (1);
	}

	i = 0;
	while (expanded[i] && expanded[i] != ' ' && expanded[i] != '\t')
		i++;
	if (expanded[i] == '\0')
	{
		res[0] = expanded;
		return (1);
	}

	/* parça 0: ilk kelime → "..." */
	q = ft_strjoin_free(ms, gc_strdup(ms, "\""),
		ft_strjoin_free(ms, gc_strndup(ms, expanded, i), gc_strdup(ms, "\"")));
	res[0] = q;

	/* parça 1: lider boşlukları at, kalan varsa → "..." */
	j = i;
	while (expanded[j] == ' ' || expanded[j] == '\t')
		j++;
	if (expanded[j] == '\0')
		return (1); /* sadece boşlukmuş, ikinci parça yok */

	rest = expanded + j; /* artık başta boşluk yok */
	q = ft_strjoin_free(ms, gc_strdup(ms, "\""),
		ft_strjoin_free(ms, gc_strdup(ms, rest), gc_strdup(ms, "\"")));
	res[1] = q;

	return (2);
}

static int	append_as_is(char *token, t_ms *ms, char **res)
{
	char	*expanded;

	expanded = expand_token(token, ms);
	if (expanded[0] == '\0' && token[0] != '"' && token[0] != '\'')
		return (0);
	res[0] = expanded;
	return (1);
}

static char	**resize_result(t_ms *ms, char **old, int old_cap, int new_cap)
{
	char	**new;
	int		i;

	new = gc_malloc(ms, sizeof(char *) * new_cap);
	if (!new)
		return (NULL);
	i = 0;
	while (i < old_cap)
	{
		new[i] = old[i];
		i++;
	}
	return (new);
}

static int	count_split_pieces(char *token, t_ms *ms)
{
	char	*expanded;
	size_t	i;
	size_t	j;

	expanded = expand_token(token, ms);
	if (!expanded)
		return (0);
	if (expanded[0] == '\0' && token[0] != '"' && token[0] != '\'')
		return (0);
	if (token[0] == '"' || token[0] == '\'')
		return (1);

	i = 0;
	while (expanded[i] && expanded[i] != ' ' && expanded[i] != '\t')
		i++;
	if (expanded[i] == '\0')
		return (1);

	j = i;
	while (expanded[j] == ' ' || expanded[j] == '\t')
		j++;
	if (expanded[j] == '\0')
		return (1);          /* ilk kelimeden sonra sadece boşluk mevcut */

	return (2);
}

static int	needs_split(int index)
{
	return (index == 0);
}

char	**expand_tokens(char **tokens, t_ms *ms)
{
	char	**result;
	int		i;
	int		j;
	int		capacity;
	int		split;

	capacity = 8;
	result = gc_malloc(ms, sizeof(char *) * capacity);
	if (!result)
		return (NULL);
	i = 0;
	j = 0;
	while (tokens[i])
	{
		split = needs_split(i);
		while (split && j + count_split_pieces(tokens[i], ms) >= capacity)
		{
			result = resize_result(ms, result, capacity, capacity * 2);
			if (!result)
				return (NULL);
			capacity *= 2;
		}
		while (!split && j + 1 >= capacity)
		{
			result = resize_result(ms, result, capacity, capacity * 2);
			if (!result)
				return (NULL);
			capacity *= 2;
		}
		if (split)
			j += append_split_if_needed(tokens[i], ms, &result[j]);
		else
			j += append_as_is(tokens[i], ms, &result[j]);
		i++;
	}
	result[j] = NULL;
	return (result);
}
