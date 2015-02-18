#!/usr/bin/perl
#
#          Copyright Andrey Moshbear 2014-2015.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)
#
# General program.

use warnings;
use strict;
use Data::Dumper;
use LWP::UserAgent;
use Currex_backend;

our $instruments_url = 'http://api-sandbox.oanda.com/v1/instruments';
our $rates_url = 'http://api-sandbox.oanda.com/v1/prices?instruments=';

sub fetch_http {
	my $_sub = ((caller(0)))[3];
	my $_psub = ((caller(1)))[3];
	my $url = shift;
	die "$_sub: missing url" if !(ref($url) eq '');
	my $ua = LWP::UserAgent->new();
	my $response = $ua->get($url);
	die "$_psub [GET $url]:".$response->status_line if !$response->is_success;
	return $response->decoded_content(charset => 'utf-8');
}

sub get_instruments {
	my $_sub = ((caller(0)))[3];
	my $data = fetch_http($instruments_url);
	## instruments
	my @instruments = sort {$a cmp $b} ($data =~ m/"([^"_]+_[^"_]+)"/g );
	return @instruments;
}

sub prune_vertices {
	my $_sub = ((caller(0)))[3];
	my $instruments = shift or die "$_sub: missing instruments input";
	return Currex_backend::prune_vertices($instruments);
}

sub get_rates {
	my $_sub = ((caller(0)))[3];
	my $instruments = shift;
	die "$_sub: input not ref to list of instruments" if !(ref($instruments) eq 'ARRAY');
	my $json = fetch_http($rates_url.join("%2C", @$instruments));
	## Fancy quasi-qson rates file structured for minimal parsing effort
	# split by object and array delimiters to get instrument blobs
	my @data = grep { $_ =~ /instrument/ } (split(/[{}]/, $json));
	#  normalization of whitespace
	s/(^[ \t\r\n]+)|([ \t\r\n]+$)|([ \t]{2,})//g foreach (@data);

	# special whitespace
	my @rates = map {
			my ($instrument) = $_ =~ m/([^"_]+_[^"_]+)"/;
			my @bidask = $_ =~ m/ ([\d.]+)/g;
			my $rate = new Currex_backend::Rate();
			%$rate = ( instrument => $instrument,
				   bid => $bidask[0],
				   ask => $bidask[1]
			);
			$_ = $rate;
			} (@data);
	return @rates;
}

sub load_graph {
	my $_sub = ((caller(0)))[3];
	my $rates = shift or die "$_sub: missing rates";
	my $lg = shift // Currex_backend::new_Labeled_graph();
	my $_discard_modified = Currex_backend::load_graph_from_rates($lg, $rates);
	return $lg;
}

sub get_best_path {
	my $_sub = ((caller(0)))[3];
	my $lg = shift or die "$_sub: missing lg handle";
	my $max_iterations = shift // -1;
	my $path = Currex_backend::best_path($lg, $max_iterations);
	return $path;
}

sub labelize {
	my $arr = shift or die "missing index array";
	my $lab = shift or die "missing label array";
	my $lab_size = $#{ $lab };
	return
		map {
			die "bad label index: $_" if ($_ > $lab_size);
			$$lab[$_];
		} (@$arr);
}
	

Currex_backend::D_set_xparam($Currex_backend::D_flag_lib 
				| $Currex_backend::D_flag_lib_throw
				| $Currex_backend::D_flag_ofp_throw);
Currex_backend::D_set_from_string("i");

Currex_backend::D_set_file("backend.d-log");

my @instr = get_instruments();
my $pv = prune_vertices(\@instr);
print "unpruned: ", $#instr, " pruned: ", $#{ $pv }, "\n";
my @rates = get_rates($pv);
my $lg = load_graph(\@rates);
my $xpath = get_best_path($lg);

my $vtx_labels = Currex_backend::Labeled_graph_labels($lg);
my $best_path = $xpath->get_path();
my $best_rate = $xpath->{lrate};

print "path: [", $#{ $best_path }, "] ",
		join("->", labelize($best_path, $vtx_labels)),
	" lrate: ",
		$best_rate,
		" (", exp(-$best_rate), "x profit)",
	"\n";

END {
	if (defined($lg)) {
		Currex_backend::delete_Labeled_graph($lg);
	}
}

