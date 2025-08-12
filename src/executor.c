/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   executor.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oakyol <oakyol@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/02 23:42:24 by oakyol            #+#    #+#             */
/*   Updated: 2025/08/12 03:51:34 by oakyol           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/minishell.h"
#include "../libft/libft.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

// static int	first_pos_unquoted(const char *s, char a, char b)
// {
// 	int	i;
// 	int	ins;
// 	int	ind;

// 	i = 0;
// 	ins = 0;
// 	ind = 0;
// 	while (s[i])
// 	{
// 		if (s[i] == '\'' && !ind)
// 			ins = !ins;
// 		else if (s[i] == '"' && !ins)
// 			ind = !ind;
// 		else if (!ins && !ind)
// 		{
// 			if (b && s[i] == a && s[i + 1] == b)
// 				return (i);
// 			/* tek op aranıyorsa çiftini (<<, >>) sayma */
// 			if (!b && s[i] == a && s[i + 1] != a)
// 				return (i);
// 		}
// 		i++;
// 	}
// 	return (-1);
// }

// static int	output_before_input_in_raw(const char *raw)
// {
// 	int	pos_out;
// 	int	pos_in;

// 	pos_out = first_pos_unquoted(raw, '>', '>');
// 	if (pos_out < 0)
// 		pos_out = first_pos_unquoted(raw, '>', 0);
// 	pos_in = first_pos_unquoted(raw, '<', 0);
// 	if (pos_out < 0)
// 		return (0);
// 	if (pos_in < 0)
// 		return (1);
// 	return (pos_out < pos_in);
// }

static void	handle_outfile(t_cmd *cmd, t_ms *ms)
{
	int	fd;

	if (cmd->append)
		fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
	else
		fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
	{
		perror(cmd->outfile);
		gc_free_all(ms);
		exit(1);
	}
	dup2(fd, STDOUT_FILENO);
	close(fd);
}

void	redirect(t_cmd *cmd, t_ms *ms)
{
	int					fd;
	//int					do_out_first;
	struct sigaction	old_int;
	struct sigaction	old_quit;
	struct sigaction	ign;

	/* 1) heredoc içeriğini ÖNCE topla; kesilirse hiçbir dosya açma */
	if (cmd->heredoc && cmd->heredoc_fd < 0)
	{
		ign.sa_handler = SIG_IGN;
		sigemptyset(&ign.sa_mask);
		ign.sa_flags = 0;
		sigaction(SIGINT, &ign, &old_int);
		sigaction(SIGQUIT, &ign, &old_quit);
		if (handle_heredoc(cmd, ms))
		{
			gc_free_all(ms);
			exit(ms->last_exit);
		}
		sigaction(SIGINT, &old_int, NULL);
		sigaction(SIGQUIT, &old_quit, NULL);
	}

	/* 2) sıra kararı: ilk unquoted '>'/ '>>' vs tek '<' (<< hariç) */
	// do_out_first = output_before_input_in_raw(ms->raw_input);
	// /* heredoc varsa output'u zorla önce yap (bash davranışı) */
	// if (cmd->heredoc && cmd->outfile)
	// 	do_out_first = 1;

	// /* 3) output önce ise, varsa önce uygula */
	// if (do_out_first && cmd->outfile)
	// 	handle_outfile(cmd, ms);

	/* 4) infile varsa dene; hata ise perror + çık */
	if (cmd->infile)
	{
		fd = open(cmd->infile, O_RDONLY);
		if (fd < 0)
		{
			perror(cmd->infile);
			gc_free_all(ms);
			exit(1);
		}
		dup2(fd, STDIN_FILENO);
		close(fd);
	}
	else if (cmd->heredoc_fd >= 0)
	{
		/* infile yoksa ve heredoc hazırlanmışsa stdin'e bağla */
		dup2(cmd->heredoc_fd, STDIN_FILENO);
		close(cmd->heredoc_fd);
		cmd->heredoc_fd = -1;
	}

	/* 5) output'u sonra yapmamız gerekiyorsa şimdi uygula */
	if (cmd->outfile)
		handle_outfile(cmd, ms);
}

void	close_all_heredocs(t_cmd *cmd)
{
	while (cmd)
	{
		if (cmd->heredoc_fd >= 0)
		{
			close(cmd->heredoc_fd);
			cmd->heredoc_fd = -1;
		}
		cmd = cmd->next;
	}
}

static void	run_single(t_cmd *cmds, t_ms *ms)
{
	pid_t		pid;
	int			status;
	char		*path;
	int			ret;
	struct stat	sb;

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	pid = fork();
	if (pid == 0)
	{
		signal(SIGPIPE, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		redirect(cmds, ms);
		if (cmds->redirect_error)
		{
			write(2, "minishell: : No such file or directory\n", 39);
			run_single_cleanup_exit(ms, 1);
		}
		if (cmds->args && cmds->args[0] && cmds->args[0][0] == '\0'
			&& !cmds->infile && !cmds->outfile && !cmds->heredoc && cmds->heredoc_fd < 0)
		{
			ft_putstr_fd("minishell: ", 2);
			ft_putstr_fd(": command not found\n", 2);
			run_single_cleanup_exit(ms, 127);
		}
		if (!cmds->args || !cmds->args[0])
			run_single_cleanup_exit(ms, 0);
		if (is_builtin(cmds->args[0]))
		{
			ret = run_builtin(cmds, ms);
			gc_free_all(ms);
			exit(ret);
		}
		path = find_path(ms, cmds->args[0], ms->env);
		if (!path)
		{
			ft_putstr_fd("minishell: ", 2);
			ft_putstr_fd(cmds->args[0], 2);
			ft_putstr_fd(": command not found\n", 2);
			run_single_cleanup_exit(ms, 127);
		}
		if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode))
		{
			ft_putstr_fd("minishell: ", 2);
			ft_putstr_fd(cmds->args[0], 2);
			ft_putstr_fd(": Is a directory\n", 2);
			run_single_cleanup_exit(ms, 126);
		}
		execve(path, cmds->args, ms->env);
		ft_putstr_fd("minishell: ", 2);
		ft_putstr_fd(cmds->args[0], 2);
		ft_putstr_fd(": ", 2);
		perror("");
		if (errno == ENOENT)
			run_single_cleanup_exit(ms, 127);
		else
			run_single_cleanup_exit(ms, 126);
	}
	waitpid(pid, &status, 0);
	signal(SIGINT, handle_sigint);
	signal(SIGQUIT, handle_sigquit);
	close_all_heredocs(cmds);
	if (WIFEXITED(status))
		ms->last_exit = WEXITSTATUS(status);
}

void	execute(t_cmd *cmds, t_ms *ms)
{
	if (!cmds)
		return ;
	/* redir-only satırda da child içinde redirect() çalışsın */
	if (!cmds->args || !*cmds->args)
	{
		if (cmds->heredoc || cmds->infile || cmds->outfile)
		{
			if (cmds->next)
			{
				run_pipeline(cmds, ms);
				return ;
			}
			run_single(cmds, ms);
		}
		return ;
	}
	if (cmds->next)
	{
		run_pipeline(cmds, ms);
		return ;
	}
	/* redir yoksa builtin'ı parent'ta; varsa child'ta */
	if (is_builtin(cmds->args[0])
		&& !cmds->infile && !cmds->outfile && !cmds->heredoc)
	{
		ms->last_exit = run_builtin(cmds, ms);
		return ;
	}
	run_single(cmds, ms);
}
