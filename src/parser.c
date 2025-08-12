/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oakyol <oakyol@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/30 21:45:15 by oakyol            #+#    #+#             */
/*   Updated: 2025/08/11 01:52:55 by oakyol           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/minishell.h"
#include "../libft/libft.h"

int	is_redirect(const char *token)
{
	return (!ft_strcmp(token, "<")
		|| !ft_strcmp(token, ">")
		|| !ft_strcmp(token, ">>")
		|| !ft_strcmp(token, "<<"));
}

char	*bash_quote_trim(const char *token, t_ms *ms)
{
	char	*res;
	int		i;
	int		j;
	char	quote;

	(void)ms;
	res = gc_malloc(ms, ft_strlen(token) + 1);
	if (!res)
	{
		write(2, "minishell: bash_quote_trim allocation failed\n", 34);
		return (NULL);
	}
	i = 0;
	j = 0;
	while (token[i])
	{
		if (token[i] == '\'' || token[i] == '"')
		{
			quote = token[i++];
			while (token[i] && token[i] != quote)
				res[j++] = token[i++];
			if (token[i] == quote)
				i++;
		}
		else
			res[j++] = token[i++];
	}
	res[j] = '\0';
	return (res);
}

// static int	parse_redirect(t_cmd *cmd, char **tokens, int *i, t_ms *ms)
// {
// 	char	*clean;
// 	char	*filename;

// 	if (cmd->parse_block)
// 	{
// 		if (!ft_strcmp(tokens[*i], "<<"))
// 			(*i) += 2;
// 		else if (!ft_strcmp(tokens[*i], "<")
// 			|| !ft_strcmp(tokens[*i], ">")
// 			|| !ft_strcmp(tokens[*i], ">>"))
// 		{
// 			(*i)++;
// 			if (tokens[*i])
// 				(*i)++;
// 		}
// 		else
// 			(*i)++;
// 		return (0);
// 	}

// 	if (!ft_strcmp(tokens[*i], "<"))
// 	{
// 		filename = tokens[++(*i)];
// 		if (!filename)
// 		{
// 			ms->last_exit = 2;
// 			return (1);
// 		}
// 		cmd->infile = gc_strdup(ms, filename);
// 		cmd->seen_input = 1;
// 		(*i)++;
// 		return (0);
// 	}
// 	else if (!ft_strcmp(tokens[*i], ">"))
// 	{
// 		filename = tokens[++(*i)];
// 		if (!filename)
// 		{
// 			ms->last_exit = 2;
// 			return (1);
// 		}
// 		cmd->outfile = gc_strdup(ms, filename);
// 		cmd->append = 0;
// 		(*i)++;
// 		return (0);
// 	}
// 	else if (!ft_strcmp(tokens[*i], ">>"))
// 	{
// 		filename = tokens[++(*i)];
// 		if (!filename)
// 		{
// 			ms->last_exit = 2;
// 			return (1);
// 		}
// 		cmd->outfile = gc_strdup(ms, filename);
// 		cmd->append = 1;
// 		(*i)++;
// 		return (0);
// 	}
// 	else if (!ft_strcmp(tokens[*i], "<<"))
// 	{
// 		clean = bash_quote_trim(tokens[*i + 1], ms);
// 		if (!clean)
// 		{
// 			ms->last_exit = 1;
// 			return (1);
// 		}
// 		add_heredoc(cmd, clean, ms);
// 		ms->heredoc_index++;
// 		cmd->heredoc = 1;
// 		(*i) += 2;
// 		return (0);
// 	}
// 	(*i)++;
// 	return (0);
// }

static int	parse_redirect(t_cmd *cmd, char **tokens, int *i, t_ms *ms)
{
	int		fd;
	char	*clean;
	char	*filename;

	if (cmd->parse_block)
	{
		if (!ft_strcmp(tokens[*i], "<<"))
			(*i) += 2;
		else if (!ft_strcmp(tokens[*i], "<")
			|| !ft_strcmp(tokens[*i], ">")
			|| !ft_strcmp(tokens[*i], ">>"))
		{
			(*i)++;
			if (tokens[*i])
				(*i)++;
		}
		else
			(*i)++;
		return (0);
	}

	if (!ft_strcmp(tokens[*i], "<"))
	{
		filename = tokens[++(*i)];
		if (!filename)
		{
			ms->last_exit = 2;
			return (1);
		}
		clean = bash_quote_trim(filename, ms);
		if (clean[0] == '\0')
		{
			ms->last_exit = 1;
			cmd->redirect_error = 1;
			(*i)++;
			return (0);
		}
		if (!cmd->infile)
		{
			cmd->infile = gc_strdup(ms, filename);
			cmd->seen_input = 1;
		}
		(*i)++;
		return (0);
	}
	else if (!ft_strcmp(tokens[*i], ">"))
	{
		filename = tokens[++(*i)];
		if (!filename)
		{
			ms->last_exit = 2;
			return (1);
		}
		clean = bash_quote_trim(filename, ms);
		if (clean[0] == '\0')
		{
			ms->last_exit = 1;
			cmd->redirect_error = 1;
			(*i)++;
			return (0);
		}
		cmd->outfile = gc_strdup(ms, filename);
		cmd->append = 0;

		/* Yalnızca heredoc görülmemiş ve '<' henüz gelmemişse
		   dosyayı burada oluştur/truncate et (bash gibi soldan-sağa dokundur) */
		if (!cmd->heredoc && !cmd->seen_input)
		{
			fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (fd >= 0)
				close(fd);
		}

		(*i)++;
		return (0);
	}
	else if (!ft_strcmp(tokens[*i], ">>"))
	{
		filename = tokens[++(*i)];
		if (!filename)
		{
			ms->last_exit = 2;
			return (1);
		}
		clean = bash_quote_trim(filename, ms);
		if (clean[0] == '\0')
		{
			ms->last_exit = 1;
			cmd->redirect_error = 1;
			(*i)++;
			return (0);
		}
		cmd->outfile = gc_strdup(ms, filename);
		cmd->append = 1;

		/* Aynı mantık: heredoc yok ve '<' henüz gelmediyse burada oluştur */
		if (!cmd->heredoc && !cmd->seen_input)
		{
			fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
			if (fd >= 0)
				close(fd);
		}

		(*i)++;
		return (0);
	}
	else if (!ft_strcmp(tokens[*i], "<<"))
	{
		clean = bash_quote_trim(tokens[*i + 1], ms);
		if (!clean)
		{
			ms->last_exit = 1;
			return (1);
		}
		add_heredoc(cmd, clean, ms);
		ms->heredoc_index++;
		cmd->heredoc = 1;
		(*i) += 2;
		return (0);
	}
	(*i)++;
	return (0);
}

int	is_quoted_operator(const char *raw_input, const char *op)
{
	int		i;
	int		inside_single;
	int		inside_double;
	size_t	op_len;

	i = 0;
	inside_single = 0;
	inside_double = 0;
	op_len = ft_strlen(op);
	while (raw_input[i])
	{
		if (raw_input[i] == '\'' && !inside_double)
			inside_single = !inside_single;
		else if (raw_input[i] == '"' && !inside_single)
			inside_double = !inside_double;
		else if (!inside_single && !inside_double
			&& !ft_strncmp(&raw_input[i], op, op_len))
			return (0);
		else if ((inside_single || inside_double)
			&& !ft_strncmp(&raw_input[i], op, op_len))
			return (1);
		i++;
	}
	return (0);
}

int	is_quoted_operator_parser(const char *raw, int target_idx)
{
	int		i;
	int		count;
	int		start;
	char	quote;

	i = 0;
	count = 0;
	while (raw[i])
	{
		while (raw[i] && (raw[i] == ' ' || raw[i] == '\t'))
			i++;
		if (!raw[i])
			break ;
		start = i;
		if (raw[i] == '\'' || raw[i] == '"')
		{
			quote = raw[i++];
			while (raw[i] && raw[i] != quote)
				i++;
			if (raw[i] == quote)
				i++;
		}
		else
		{
			while (raw[i] && raw[i] != ' ' && raw[i] != '\t')
			{
				if (raw[i] == '\'' || raw[i] == '"')
				{
					quote = raw[i++];
					while (raw[i] && raw[i] != quote)
						i++;
					if (raw[i] == quote)
						i++;
				}
				else
					i++;
			}
		}
		if (count == target_idx)
			return (raw[start] == '\'' || raw[start] == '"');
		count++;
	}
	return (0);
}

t_cmd	*parser(char **tokens, t_ms *ms)
{
	t_cmd	*cmds;
	t_cmd	*current;
	int		i;
	int		cmd_start;
	int		has_heredoc;

	cmds = NULL;
	i = 0;
	while (tokens[i])
	{
		current = init_cmd(ms);
		cmd_start = i;
		has_heredoc = 0;
		while (tokens[i])
		{
			if (!ft_strcmp(tokens[i], "|")
				&& !is_quoted_operator_parser(ms->raw_input, i))
				break ;
			if (is_redirect(tokens[i])
				&& !is_quoted_operator_parser(ms->raw_input, i))
			{
				if (!ft_strcmp(tokens[i], "<<"))
					has_heredoc = 1;
				parse_redirect(current, tokens, &i, ms);
				continue ;
			}
			if (is_redirect(tokens[i])
				&& is_quoted_operator_parser(ms->raw_input, i)
				&& i == cmd_start)
			{
				write(2, "minishell: ", 11);
				write(2, tokens[i], ft_strlen(tokens[i]));
				write(2, ": command not found\n", 21);
				ms->last_exit = 127;
				return (NULL);
			}
			i++;
		}
		current->args = copy_args(tokens, cmd_start, i, ms);
		if (has_heredoc || current->args)
			add_cmd(&cmds, current);
		if (tokens[i])
			i++;
	}
	return (cmds);
}
