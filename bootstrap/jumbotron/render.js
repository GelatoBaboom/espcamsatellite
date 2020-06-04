function Records(chartElId) {
	this.me = new Records_CLASS();
	this.me.init(chartElId);
	return this.me;
}
Records.prototype.action = function () {
	//return this.click;
}
function Records_CLASS() {
	return {
		chartEl: null,
		graphRendered: false,
		lineChart: null,
		registers: null,
		init: function (chartElId) {
			this.chartEl = $('#' + chartElId);
		},
		chartCnf: {
			responsive: true,
			aspectRatio: 1,
			hoverMode: 'index',
			stacked: false,
			title: {
				display: true,
				text: 'Grafico de temperatura'
			},
			scales: {
				yAxes: [{
						type: "linear", // only linear but allow scale type registration. This allows extensions to exist solely for log scale for instance
						display: true,
						position: "left",
						id: "y-axis-1",
					}
				],
			}
		},
		speedData: {
			labels: [],
			datasets: [{
					label: "Temperatura",
					data: [],
					borderColor: "#0368AE",
					lineTension: 0.3,
					fill: 'start',
					yAxisID: "y-axis-1",
				}
			]

		},
		render: function (data) {
			var spData = this.speedData;
			var chrt = this.chartCnf;
			spData.labels = data.labels;
			spData.datasets[0].data = data.values;
			this.lineChart = new Chart(this.chartEl, {
					type: 'line',
					data: spData,
					options: chrt
				});
			this.graphRendered = true;
		},
		updateChart: function (data) {
			this.lineChart.data.labels = data.labels;
			this.lineChart.data.datasets[0].data = data.values;
			this.lineChart.update();

		},
		renderTempGraph: function () {
			var thiscomp = this;
			if (this.chartEl.length) {
				$.ajax({
					type: 'GET',
					dataType: "json",
					url: '/getTempJson?t=236',
					processData: true,
					async: false,
					success: function (resp) {
						if (!thiscomp.graphRendered) {
							thiscomp.render(resp);
						} else {
							thiscomp.updateChart(resp);
						}
					}
				});

			}
		},
		loadRegsBase: function () {
			$.ajax({
				type: 'GET',
				dataType: "json",
				url: '/getRegs',
				processData: true,
				async: false,
				success: function (resp) {
					registers = resp.registers;
				}
			});
		}

	}
}
$(document).ready(function () {
	var r = new Records('tempChart');
	r.registers = [{
			"year": 2019,
			"months": [{
					"month": 6,
					"days": ["4"]
				}
			]
		}, {
			"year": 120,
			"months": [{
					"month": 6,
					"days": ["4"]
				}
			]
		}, {
			"year": 2020,
			"months": [{
					"month": 6,
					"days": ["1", "2", "3", "4"]
				}, {
					"month": 4,
					"days": ["2", "3"]
				}, {
					"month": 5,
					"days": ["10", "11"]
				}
			]
		}
	];
	console.log(r.registers[0].year);
	console.log(r.registers[0].months[r.registers[0].months.length - 1].month);
	console.log(r.registers[0].months[r.registers[0].months.length - 1].days[r.registers[0].months[r.registers[0].months.length - 1].days.length - 1]);

	r.render({
		"values": ["21.5", "21.6", "20.3"],
		"labels": ["10:50", "10:51", "10:52"]
	});
});
