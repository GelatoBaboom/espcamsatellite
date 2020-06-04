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
		selectedReg: {
			year: 0,
			month: 0,
			day: 0,
		},
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
					url: '/getTempJson',
					data: {
						y: thiscomp.selectedReg.year,
						m: thiscomp.selectedReg.month,
						d: thiscomp.selectedReg.day
					},
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
		renderRegList: function () {
			var thiscomp = this;
			var regList = $('#regList');
			$.each(thiscomp.registers, function (idx, yvalue) {
				var yliEl = $('<li>' + yvalue.year + '</li>');
				yulEl = $('<ul/>');
				$.each(yvalue.months, function (idx, mvalue) {
					var mliEl = $('<li>' + mvalue.month + '</li>');
					mulEl = $('<ul/>');
					$.each(mvalue.days, function (idx, dvalue) {
						var dliEl = $('<li>' + dvalue + '</li>');
						dliEl.click(function () {
							thiscomp.selectedReg.year = yvalue.year;
							thiscomp.selectedReg.month = mvalue.month;
							thiscomp.selectedReg.day = dvalue;
							thiscomp.renderTempGraph();
						});

						mulEl.append(dliEl);
					});
					mliEl.append(mulEl);
					yulEl.append(mliEl);

				});
				yliEl.append(yulEl);
				regList.append(yliEl);

			});

		},
		loadRegsBase: function () {
			var thiscomp = this;
			$.ajax({
				type: 'GET',
				dataType: "json",
				url: '/getRegs',
				processData: true,
				async: false,
				success: function (resp) {
					thiscomp.registers = resp.registers;
				}
			});
		}

	}
}
$(document).ready(function () {
	var r = new Records('tempChart');
	r.loadRegsBase();
	console.log(r);
	r.selectedReg.year = r.registers[r.registers.length - 1].year;
	r.selectedReg.month = r.registers[0].months[0].month;
	r.selectedReg.day = r.registers[0].months[r.registers[0].months.length - 1].days[r.registers[0].months[r.registers[0].months.length - 1].days.length - 1];
	r.renderRegList();
	r.renderTempGraph();

	var interval = setInterval(function () {
			r.renderTempGraph();
		}, 10000);
});
