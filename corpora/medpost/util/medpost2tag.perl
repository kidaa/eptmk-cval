
# Convert medpost tags to an alternate tag set
# the translations are stored in a translation "lexicon"
# and allow for
# 1. multiword: multiword tokens,
# 2. explicit: words with fixed tag assignments, regardless of tag
# 3. conditional: words with fixed tag assignments, depending on tag
#    (cannot overlap with explicit words)
# 4. general: tags with fixed tag equivalents
# These are applied in this order, the idea is that
# multi-word terms are translated to medpost or target tags
# then any explicit word or word-tag translations are made,
# and finally, any tags that remain to be translated are.

$option_target = "penn";
$option_lexicon = "medpost.penn";
$option_tagsep = "/";
$option_preserve = 0;
$option_ditto = 0;

$option_parser = "";

$option_homedir = $0;
$option_homedir =~ s/[^\/]*$//g;
$option_homedir = "." if ($option_homedir eq "");

while (substr($ARGV[0], 0, 1) eq "-")
{
	$option_preserve = 1 if ($ARGV[0] eq "-preserve");
	$option_ditto = 1 if ($ARGV[0] eq "-ditto");

	$option_parser = "stanford" if ($ARGV[0] eq "-stanford");
	$option_parser = "rasp" if ($ARGV[0] eq "-rasp");
	$option_parser = "collins" if ($ARGV[0] eq "-collins");
	$option_parser = "bikel" if ($ARGV[0] eq "-bikel");

	if ($ARGV[0] eq "-penn")
	{
		$option_target = "penn";
		$option_lexicon = "medpost.penn";
		$option_tagsep = "/";
	}
	if ($ARGV[0] eq "-claws2")
	{
		$option_target = "claws2";
		$option_lexicon = "medpost.claws2";
		$option_tagsep = "_";
	}
	if ($ARGV[0] eq "-home")
	{
		shift @ARGV;
		$option_homedir = $ARGV[0];
	}
	shift @ARGV;
}

if (($option_parser eq "stanford" || $option_parser eq "collins" || $option_parser eq "bikel")
	&& $option_target ne "penn")
{
	print "$option_parser parser requires penn tagset.\n";
	exit;
}

if ($option_parser eq "rasp" && $option_target ne "claws2")
{
	print "$option_parser parser requires claws2 tagset.\n";
	exit;
}

%multiwords = ();
%explicit = ();
%conditional = ();
%general = ();

for (`cat $option_homedir/$option_lexicon`)
{
	chomp;
	if (/^\[(.*)\]/) { $var = $1; }
	if (/\s*(.*?)\s*\=\>\s*(.*)/) { $$var{$1} = $2; }
}

READ:
while (<>)
{
	chomp;
	s/\s+/ /g;

	@x = split(/\s+/, $_);

	# First, gather the ditto tags

	$last_ditto = 0;
	for ($i = 0; $i < @x; $i++)
	{
		$ditto[$i] = "";
		($w, $t) = split(/_/, $x[$i]);

		# if it's not a tagged word, skip this line

		if ($t eq "")
		{
			print "$_\n" if ($option_preserve);
			next READ;
		}

		$save_tag[$i] = $t;

		if ($t =~ /\+$/)
		{
			$x[$i] =~ s/\+$//;
			$ditto[$i] = 1;
			$last_ditto = 1;
		} elsif ($last_ditto)
		{
			# get the start of the ditto

			$ditto_num = 1;
			for ($j = $i - 1; $j >= 0 && $ditto[$j]; $j--)
			{
				$ditto_num++;
			}

			$k = 1;
			for ($j++; $ditto[$j]; $j++)
			{
				$ditto[$j] = "$ditto_num$k";
				$k++;
			}
			$ditto[$j] = "$ditto_num$k";

			$last_ditto = 0;
		} else
		{
			$last_ditto = 0;
		}
	}

	# Translate each token seperately

	$last_map = -1;
	for ($i = 0; $i < @x; $i++)
	{
		# translate a phrase first

		if ($i > $last_map)
		{
			for ($j = $i + 3; $j > $i; $j--)
			{
				next if ($j > $#x);
				$p = join(" ", @x[$i..$j]);
				$p =~ s/_[^ ]+//g;
				$p = lc($p);
				if ($multiwords{$p})
				{
					@t = split(/[ ]/, $multiwords{$p});
					if (@t != ($j - $i + 1))
					{
						print "error on $p\n";
						next;
					}

					for ($k = 0; $i + $k <= $j; $k++)
					{
						if ($t[$k] ne "-")
						{
							$x[$i+$k] =~ s/_.*/_$t[$k]/;
						}
					}

					$last_map = $j;
					last;
				}
			}
		}

		# 1. explicit tag assignments

		for $w (keys %explicit)
		{
			$t = $explicit{$w};
			$w = quotemeta($w);
			if ($x[$i] =~ /^${w}_/i)
			{
				$x[$i] =~ s/_.*/_$t/;
				last;
			}
		}

		# 2. conditional tag assignments

		for $c (keys %conditional)
		{
			$t = $conditional{$c};
			$c = quotemeta($c);
			if ($x[$i] =~ /^$c$/i)
			{
				$x[$i] =~ s/_.*/_$t/;
				last;
			}
		}

		# 3. generally valid tag substitutions

		for $t (keys %general)
		{
			$s = $general{$t};
			$save_tag[$i] = $s if ($save_tag[$i] eq $t);

			$t = quotemeta($t);
			if ($x[$i] =~ /_$t$/) { $x[$i] =~ s/_.*/_$s/; }
		}


		# 4. should detect count nouns and tag them NN1, but that's too much trouble

	}

	# Despite all that, put the ditto-ed tags back

	if ($option_ditto)
	{
		$dt = "";
		for ($i = @x - 1; $i >= 0; $i--)
		{
			if ($ditto[$i])
			{
				if (! $dt)
				{
					$dt = $save_tag[$i];
				}
				$x[$i] =~ s/_.*/_$dt/;
			} else
			{
				$dt = "";
			}
		}
	}

	if ($option_parser eq "stanford" || $option_parser eq "collins" || $option_parser eq "bikel")
	{
		for (@x)
		{
			s/[(]/-LRB-/g;
			s/[)]/-RRB-/g;
			s/[[]/-LSB-/g;
			s/[]]/-RSB-/g;
			s/[{]/-LCB-/g;
			s/[}]/-RCB-/g;
		}
	}

	if ($option_parser eq "collins")
	{
		$sent = @x;
		$sent .= " " . join(" ", @x);
		$sent =~ s/_/ /g;
	} elsif ($option_parser eq "bikel")
	{
		$sent = "(";
		for ($i = 0; $i < @x; $i++)
		{
			$sent .= " " if ($i > 0);
			($w, $t) = split(/_/, $x[$i]);
			$t .= $ditto[$i] if ($option_ditto);
			$sent .= "($w ($t))";
		}
		$sent .= ")";
	} else
	{
		$sent = "";
		for ($i = 0; $i < @x; $i++)
		{
			$sent .= " " if ($i > 0);
			$sent .= "$x[$i]";
			$sent .= $ditto[$i] if ($option_ditto);
		}
	}

	$sent = "^_^ $sent" if ($option_parser eq "rasp");

	if ($option_tagsep ne "_")
	{
		$sent =~ s/_/$option_tagsep/g;
	}

	print "$sent\n";
}
