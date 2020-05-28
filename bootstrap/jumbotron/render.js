
var chartCnf = {
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
};
var speedData = {
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

};
var graphRendered = false;
var lineChart = null;
function render(data) {
    speedData.labels = data.labels;
    speedData.datasets[0].data = data.values;
    lineChart = new Chart($("#tempChart"), {
        type: 'line',
        data: speedData,
        options: chartCnf
    });
    graphRendered = true;
}
function updateChart(data) {
    lineChart.data.labels = data.labels;
    lineChart.data.datasets[0].data = data.values;
    lineChart.update();

}
$(document).ready(function () {
	 render({"values":["21.5","21.6"],"labels":["10:50","10:51"]});
    /* var interval = setInterval(function () {
        if ($('#tempChart').length) {
            $.ajax({
                type: 'GET',
                dataType: "json",
                url: '/getJson.ashx',
                processData: true,
                async: false,
                success: function (resp) {
                    if (!graphRendered) {
                        render(resp);
                    } else {
                        updateChart(resp);
                    }
                }
            });

        }
    }, 2000); */
});
